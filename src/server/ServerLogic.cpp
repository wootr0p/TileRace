// ServerLogic.cpp — loop ENet autoritativo condiviso tra TileRace_Server e LocalServer.
// Estratto da server/main.cpp (passo 20) per supportare la modalità offline.

#include "ServerLogic.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <enet/enet.h>
#include "Protocol.h"
#include "World.h"
#include "Player.h"
#include "Physics.h"

void RunServer(uint16_t port, const char* map_path, std::atomic<bool>& stop_flag) {
    printf("[server] TileRace v%s  (protocol %u)\n", GAME_VERSION, PROTOCOL_VERSION);
    // Percorso e livello iniziali: usati per il reset a fine sessione.
    const std::string initial_map_path(map_path);
    int initial_level = 1;
    {
        const char* p = std::strstr(map_path, "level_");
        if (p) std::sscanf(p + 6, "%d", &initial_level);
    }

    // --- Stato sessione ---
    // in_lobby:    true mentre siamo nella _lobby.txt (nessun time limit, countdown speciale)
    // game_locked: true durante la partita reale → nuove connessioni vengono rifiutate
    // session_token: ID univoco rigenerato a ogni nuova sessione, inviato via PktWelcome
    bool     in_lobby      = (std::strstr(map_path, "_lobby") != nullptr);
    bool     game_locked   = false;
    uint32_t session_token = 0u;
    // Fase classifica: true tra l'invio di PktLevelResults e il successivo do_level_change.
    bool     in_results       = false;
    uint32_t results_start_ms = 0u;
    std::unordered_set<ENetPeer*> ready_peers;

    World world;
    float spawn_x = 0.f, spawn_y = 0.f;
    int   current_level = in_lobby ? 0 : initial_level;

    auto load_level = [&](const char* path) -> bool {
        World tmp;
        if (!tmp.LoadFromFile(path)) return false;
        world = tmp;
        spawn_x = 0.f; spawn_y = 0.f;
        for (int ty = 0; ty < world.GetHeight(); ty++)
            for (int tx = 0; tx < world.GetWidth(); tx++)
                if (world.GetTile(tx, ty) == 'X') {
                    spawn_x = static_cast<float>(tx * TILE_SIZE);
                    spawn_y = static_cast<float>(ty * TILE_SIZE);
                }
        return true;
    };

    if (!load_level(map_path)) {
        fprintf(stderr, "[server] ERRORE: mappa non trovata: %s\n", map_path);
        return;
    }
    printf("[server] mappa caricata (%d x %d tile)\n",
           world.GetWidth(), world.GetHeight());

    ENetAddress address{};
    address.host = ENET_HOST_ANY;
    address.port = port;

    ENetHost* server = enet_host_create(
        &address,
        static_cast<size_t>(MAX_CLIENTS),
        static_cast<size_t>(CHANNEL_COUNT),
        0, 0
    );
    if (!server) {
        fprintf(stderr,
                "[server] ERRORE: enet_host_create fallita (porta %u occupata?)\n", port);
        return;
    }
    printf("[server] in ascolto su UDP porta %u  (max %zu client)\n",
           port, static_cast<size_t>(MAX_CLIENTS));

    uint32_t next_player_id = 1;
    std::unordered_map<ENetPeer*, Player>   players;
    // Miglior tempo di finish per ogni player nel livello corrente.
    // Non viene azzerato da PKT_RESTART: un giocatore che ha finito e riparte
    // conserva il proprio risultato per la classifica di fine livello.
    // 0 = non ha ancora finito questo livello.
    std::unordered_map<ENetPeer*, uint32_t> player_best_ticks;

    // Tempo ENet (ms) in cui TUTTI i player sono entrati nella zona finale.
    // 0 = nessuno o non tutti dentro. Solo il loop esterno lo aggiorna (una volta
    // per iterazione di ~16 ms) per evitare falsi reset da jitter fisico.
    uint32_t zone_start_ms  = 0;
    uint32_t level_start_ms = enet_time_get();   // inizio livello corrente
    static constexpr uint32_t LEVEL_TIME_LIMIT_MS = 120000; // 2 minuti
    static constexpr uint32_t NEXT_LEVEL_MS       = 3000;
    static constexpr uint32_t RESULTS_DURATION_MS = 15000;  // 15 s schermata classifica

    // Controlla se tutti i player hanno almeno un angolo su un tile 'E'.
    // Restituisce false se non ci sono player.
    auto all_in_zone = [&]() -> bool {
        if (players.empty()) return false;
        for (auto& [peer, pl] : players) {
            (void)peer;
            const PlayerState& s = pl.GetState();
            const int tx0 = static_cast<int>(s.x)               / TILE_SIZE;
            const int ty0 = static_cast<int>(s.y)               / TILE_SIZE;
            const int tx1 = static_cast<int>(s.x + TILE_SIZE - 1.f) / TILE_SIZE;
            const int ty1 = static_cast<int>(s.y + TILE_SIZE - 1.f) / TILE_SIZE;
            const bool on_e = world.GetTile(tx0, ty0) == 'E' ||
                              world.GetTile(tx1, ty0) == 'E' ||
                              world.GetTile(tx0, ty1) == 'E' ||
                              world.GetTile(tx1, ty1) == 'E';
            if (!on_e) return false;
        }
        return true;
    };

    // Calcola il valore next_level_countdown_ticks da trasmettere ai client.
    // 0 = countdown non attivo.
    auto countdown_ticks = [&]() -> uint32_t {
        if (zone_start_ms == 0) return 0;
        const uint32_t elapsed = enet_time_get() - zone_start_ms;
        if (elapsed >= NEXT_LEVEL_MS) return 0;
        return (NEXT_LEVEL_MS - elapsed) * 60u / 1000u;
    };

    // Costruisce e broadcast il PktLevelResults, poi entra in fase classifica.
    auto send_results = [&](const char* reason) {
        PktLevelResults res_pkt{};
        res_pkt.level = static_cast<uint8_t>(current_level);
        std::vector<ResultEntry> entries;
        for (auto& [p2, pl2] : players) {
            const PlayerState& s2 = pl2.GetState();
            ResultEntry e{};
            e.player_id = s2.player_id;
            std::strncpy(e.name, s2.name, sizeof(e.name) - 1);
            // Usa il miglior finish salvato per questo livello, non lo stato
            // attuale: un giocatore che ha finito e poi ha premuto restart non
            // viene penalizzato con DNF nella classifica.
            const auto best_it = player_best_ticks.find(p2);
            if (best_it != player_best_ticks.end() && best_it->second > 0) {
                e.finished    = 1u;
                e.level_ticks = best_it->second;
            } else {
                e.finished    = 0u;
                e.level_ticks = s2.level_ticks;  // tempo parziale per i DNF
            }
            entries.push_back(e);
        }
        std::sort(entries.begin(), entries.end(),
            [](const ResultEntry& a, const ResultEntry& b) {
                if (a.finished != b.finished) return a.finished > b.finished;
                return a.level_ticks < b.level_ticks;
            });
        res_pkt.count = static_cast<uint8_t>(entries.size());
        for (size_t i2 = 0; i2 < entries.size() && i2 < static_cast<size_t>(MAX_PLAYERS); i2++)
            res_pkt.entries[i2] = entries[i2];
        ENetPacket* resl = enet_packet_create(&res_pkt, sizeof(res_pkt), ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(server, CHANNEL_RELIABLE, resl);
        enet_host_flush(server);
        in_results       = true;
        results_start_ms = enet_time_get();
        ready_peers.clear();
        printf("[server] RESULTS (%s) level=%d players=%zu\n",
               reason, current_level, entries.size());
    };

    ENetEvent event;
    while (!stop_flag) {
        while (enet_host_service(server, &event, 16) > 0) {
            switch (event.type) {

            case ENET_EVENT_TYPE_CONNECT: {
                // Rifiuta nuove connessioni se la partita è già in corso.
                if (game_locked) {
                    printf("[server] CONNECT rifiutato (partita in corso) %08x:%u\n",
                           event.peer->address.host, event.peer->address.port);
                    enet_peer_disconnect(event.peer, DISCONNECT_SERVER_BUSY);
                    break;
                }
                // Genera il token di sessione al primo connect (identifica questa sessione).
                if (session_token == 0u)
                    session_token = enet_time_get() ^
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event.peer) & 0xFFFFFFFFu);

                PlayerState ps{};
                ps.player_id = next_player_id++;
                ps.x = spawn_x;
                ps.y = spawn_y;
                Player pl{};
                pl.SetState(ps);
                players[event.peer] = pl;

                // Informa il client del suo player_id e del token di sessione.
                PktWelcome welcome{};
                welcome.player_id     = ps.player_id;
                welcome.session_token = session_token;
                ENetPacket* wlc = enet_packet_create(&welcome, sizeof(welcome),
                                                     ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(event.peer, CHANNEL_RELIABLE, wlc);
                printf("[server] CONNECT player_id=%u  session=%u  %08x:%u\n",
                       ps.player_id, session_token,
                       event.peer->address.host,
                       event.peer->address.port);
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE: {
                const size_t len = event.packet->dataLength;
                if (len >= 1) {
                    const uint8_t type = event.packet->data[0];

                    // Input da simulare.
                    if (type == PKT_INPUT && len >= sizeof(PktInput)) {
                        PktInput pkt{};
                        std::memcpy(&pkt, event.packet->data, sizeof(PktInput));
                        auto it = players.find(event.peer);
                        if (it != players.end()) {
                            it->second.Simulate(pkt.frame, world);

                            // Timer di livello e latch "finished" (first-touch, 4 angoli).
                            PlayerState s = it->second.GetState();
                            if (!s.finished) {
                                s.level_ticks++;
                                const int tx0 = static_cast<int>(s.x) / TILE_SIZE;
                                const int ty0 = static_cast<int>(s.y) / TILE_SIZE;
                                const int tx1 = static_cast<int>(s.x + TILE_SIZE - 1.f) / TILE_SIZE;
                                const int ty1 = static_cast<int>(s.y + TILE_SIZE - 1.f) / TILE_SIZE;
                                if (world.GetTile(tx0, ty0) == 'E' ||
                                    world.GetTile(tx1, ty0) == 'E' ||
                                    world.GetTile(tx0, ty1) == 'E' ||
                                    world.GetTile(tx1, ty1) == 'E') {
                                    s.finished = true;
                                    printf("[server] FINISH player_id=%u ticks=%u\n",
                                           s.player_id, s.level_ticks);
                                    // Salva il risultato per la classifica: non viene
                                    // sovrascritto da restart, solo da un nuovo finish
                                    // più veloce nello stesso livello.
                                    uint32_t& best = player_best_ticks[event.peer];
                                    if (best == 0 || s.level_ticks < best)
                                        best = s.level_ticks;
                                }
                            }
                            it->second.SetState(s);

                            // --- Aggiorna zona e controlla cambio livello ---
                            // Fatto qui (dentro PKT_INPUT) con wall-clock enet_time_get:
                            // zone_start_ms viene impostato UNA SOLA VOLTA (quando tutti
                            // i player entrano) e rimane fisso finché qualcuno non esce.
                            // Leggere enet_time_get() N volte al secondo è corretto:
                            // non si accumula nulla, si confronta solo con un punto fisso.
                            if (all_in_zone()) {
                                if (zone_start_ms == 0)
                                    zone_start_ms = enet_time_get();
                            } else {
                                zone_start_ms = 0;
                            }
                            PktGameState gs_pkt{};
                            gs_pkt.state.count = 0;
                            for (auto& [p, pl] : players) {
                                if (gs_pkt.state.count < static_cast<uint32_t>(MAX_PLAYERS))
                                    gs_pkt.state.players[gs_pkt.state.count++] = pl.GetState();
                            }
                            gs_pkt.state.next_level_countdown_ticks = countdown_ticks();
                            gs_pkt.state.is_lobby = in_lobby ? 1u : 0u;
                            if (!in_lobby && !in_results) {
                                const uint32_t el = enet_time_get() - level_start_ms;
                                gs_pkt.state.time_limit_secs = el < LEVEL_TIME_LIMIT_MS
                                    ? (LEVEL_TIME_LIMIT_MS - el) / 1000u
                                    : 0u;
                            }
                            ENetPacket* bcast = enet_packet_create(
                                &gs_pkt, sizeof(gs_pkt), 0);
                            enet_host_broadcast(server, CHANNEL_RELIABLE, bcast);
                            enet_host_flush(server);

                            // --- Scatta il cambio livello se il timer zona è scaduto ---
                            if (zone_start_ms != 0 && !players.empty() &&
                                enet_time_get() - zone_start_ms >= NEXT_LEVEL_MS) {
                                zone_start_ms = 0;
                                if (in_lobby) {
                                    goto do_level_change;
                                } else if (!in_results) {
                                    send_results("zona");
                                }
                            }
                            // --- Scatta la fase results per scadenza tempo (2 min) ---
                            if (!in_lobby && !in_results && !players.empty() &&
                                enet_time_get() - level_start_ms >= LEVEL_TIME_LIMIT_MS) {
                                zone_start_ms = 0;
                                send_results("timeout");
                            }
                        }
                    }
                    // Aggiornamento nome player + controllo versione protocollo.
                    else if (type == PKT_PLAYER_INFO && len >= sizeof(PktPlayerInfo)) {
                        PktPlayerInfo info{};
                        std::memcpy(&info, event.packet->data, sizeof(PktPlayerInfo));

                        // --- Controllo versione ---
                        if (info.protocol_version != PROTOCOL_VERSION) {
                            printf("[server] VERSION MISMATCH peer=%08x client=%u server=%u --> disconnesso\n",
                                   event.peer->address.host,
                                   info.protocol_version, PROTOCOL_VERSION);
                            // 1) Pacchetto applicativo: contiene la versione del server.
                            PktVersionMismatch vm{};
                            ENetPacket* vmpkt = enet_packet_create(&vm, sizeof(vm),
                                                                   ENET_PACKET_FLAG_RELIABLE);
                            enet_peer_send(event.peer, CHANNEL_RELIABLE, vmpkt);
                            // 2) Disconnessione morbida: ENet la esegue dopo aver
                            //    consegnato tutti i pacchetti reliable pendenti.
                            enet_peer_disconnect(event.peer, DISCONNECT_VERSION_MISMATCH);
                            players.erase(event.peer);
                            break;
                        }

                        auto it = players.find(event.peer);
                        if (it != players.end()) {
                            PlayerState s = it->second.GetState();
                            std::strncpy(s.name, info.name, sizeof(s.name) - 1);
                            s.name[sizeof(s.name) - 1] = '\0';
                            it->second.SetState(s);
                            printf("[server] PLAYER_INFO id=%u name='%s'\n",
                                   s.player_id, s.name);
                        }
                    }
                    // Ripartenza dal via.
                    else if (type == PKT_RESTART && len >= sizeof(PktRestart)) {
                        auto it = players.find(event.peer);
                        if (it != players.end()) {
                            PlayerState s = it->second.GetState();
                            s.x = spawn_x;  s.y = spawn_y;
                            s.vel_x = 0.f;  s.vel_y = 0.f;
                            s.move_vel_x = 0.f;
                            s.level_ticks = 0;
                            s.finished    = false;
                            it->second.SetState(s);
                            printf("[server] RESTART player_id=%u\n", s.player_id);
                        }
                    }
                    // Giocatore pronto per il livello successivo (durante fase results).
                    else if (type == PKT_READY && in_results) {
                        ready_peers.insert(event.peer);
                        const auto it2 = players.find(event.peer);
                        const uint32_t pid = (it2 != players.end())
                            ? it2->second.GetState().player_id : 0u;
                        printf("[server] READY player_id=%u  (%zu/%zu)\n",
                               pid, ready_peers.size(), players.size());
                        if (ready_peers.size() >= players.size()) {
                            in_results = false;
                            ready_peers.clear();
                            goto do_level_change;
                        }
                    }
                }
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("[server] DISCONNECT %08x:%u\n",
                       event.peer->address.host,
                       event.peer->address.port);
                players.erase(event.peer);
                player_best_ticks.erase(event.peer);
                ready_peers.erase(event.peer);
                // Se in results e tutti i rimanenti sono già ready, salta subito.
                if (in_results && !players.empty() && ready_peers.size() >= players.size()) {
                    in_results = false;
                    ready_peers.clear();
                    goto do_level_change;
                }
                // Se non ci sono più player, resetta alla lobby e riapri le connessioni.
                if (players.empty()) {
                    in_results    = false;
                    ready_peers.clear();
                    player_best_ticks.clear();
                    in_lobby      = (std::strstr(initial_map_path.c_str(), "_lobby") != nullptr);
                    game_locked   = false;
                    session_token = 0u;
                    current_level = in_lobby ? 0 : initial_level;
                    load_level(initial_map_path.c_str());
                    zone_start_ms  = 0;
                    level_start_ms = enet_time_get();
                    next_player_id = 1;
                    printf("[server] tutti disconnessi --> reset a '%s'\n",
                           initial_map_path.c_str());
                }
                break;

            default:
                break;
            }
        }

        // Scatta il cambio livello quando scade il timer della fase results.
        if (in_results && !players.empty() &&
            enet_time_get() - results_start_ms >= RESULTS_DURATION_MS) {
            in_results = false;
            ready_peers.clear();
            goto do_level_change;
        }
        // Continua al prossimo ciclo (il cambio livello avviene via goto dal PKT_INPUT o dal timer results).
        continue;

        do_level_change:
            in_results = false;
            ready_peers.clear();
            player_best_ticks.clear();  // nuovi risultati per il prossimo livello
            // Lobby → primo livello reale: blocca nuove connessioni per questa sessione.
            if (in_lobby) {
                in_lobby    = false;
                game_locked = true;
                current_level = 1;
                printf("[server] LOBBY COMPLETE --> GAME  session_token=%u\n", session_token);
            } else {
                current_level++;
            }
            char next_path[128];
            std::snprintf(next_path, sizeof(next_path),
                          "assets/levels/level_%02d.txt", current_level);
            PktLoadLevel ll_pkt{};
            if (load_level(next_path)) {
                // Riporta tutti i player allo spawn del nuovo livello.
                for (auto& [peer, pl] : players) {
                    PlayerState s = pl.GetState();
                    s.x = spawn_x;  s.y = spawn_y;
                    s.vel_x = 0.f;  s.vel_y = 0.f;
                    s.move_vel_x = 0.f;
                    s.level_ticks = 0;
                    s.finished    = false;
                    pl.SetState(s);
                }
                ll_pkt.is_last = 0;
                std::strncpy(ll_pkt.path, next_path, sizeof(ll_pkt.path) - 1);
                printf("[server] LEVEL CHANGE --> %s\n", next_path);
            } else {
                // Ultimo livello completato: notifica i client, poi resetta
                // il server allo stato iniziale senza uscire dal loop.
                // Così sia il server dedicato che la LocalServer restano
                // pronti per una nuova sessione senza bisogno di riavvio.
                ll_pkt.is_last = 1;
                printf("[server] all levels complete --> reset to level %d\n", initial_level);
            }
            ENetPacket* ll = enet_packet_create(&ll_pkt, sizeof(ll_pkt),
                                                ENET_PACKET_FLAG_RELIABLE);
            enet_host_broadcast(server, CHANNEL_RELIABLE, ll);
            enet_host_flush(server);
            zone_start_ms  = 0;
            level_start_ms = enet_time_get();

            if (ll_pkt.is_last) {
                // Disconnetti tutti i peer e svuota le mappe: la prossima
                // connessione ripartirà dal livello 1 come una sessione nuova.
                for (auto& [peer, pl] : players)
                    enet_peer_disconnect_now(peer, 0);
                players.clear();
                // Ricarica la lobby: riapri le connessioni per una nuova sessione.
                in_lobby      = (std::strstr(initial_map_path.c_str(), "_lobby") != nullptr);
                game_locked   = false;
                session_token = 0u;
                current_level = in_lobby ? 0 : initial_level;
                load_level(initial_map_path.c_str());
                level_start_ms = enet_time_get();
                printf("[server] reset completato, lobby riaperta\n");
            }
    }

    enet_host_destroy(server);
    printf("[server] fermato\n");
}
