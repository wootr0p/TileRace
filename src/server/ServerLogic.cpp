// ServerLogic.cpp — loop ENet autoritativo condiviso tra TileRace_Server e LocalServer.
// Estratto da server/main.cpp (passo 20) per supportare la modalità offline.

#include "ServerLogic.h"
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <enet/enet.h>
#include "Protocol.h"
#include "World.h"
#include "Player.h"
#include "Physics.h"

void RunServer(uint16_t port, const char* map_path, std::atomic<bool>& stop_flag) {
    World world;
    if (!world.LoadFromFile(map_path)) {
        fprintf(stderr, "[server] ERRORE: mappa non trovata: %s\n", map_path);
        return;
    }
    printf("[server] mappa caricata (%d x %d tile)\n",
           world.GetWidth(), world.GetHeight());

    // Trova la posizione di spawn ('X') nella tilemap.
    float spawn_x = 0.f, spawn_y = 0.f;
    for (int ty = 0; ty < world.GetHeight(); ty++)
        for (int tx = 0; tx < world.GetWidth(); tx++)
            if (world.GetTile(tx, ty) == 'X') {
                spawn_x = static_cast<float>(tx * TILE_SIZE);
                spawn_y = static_cast<float>(ty * TILE_SIZE);
            }

    // Estrae il numero del livello corrente dal nome del file (es. "level_03.txt" → 3).
    int current_level = 1;
    {
        const char* p = std::strstr(map_path, "level_");
        if (p) std::sscanf(p + 6, "%d", &current_level);
    }

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

    uint32_t next_player_id = 1;   // 0 = "sconosciuto" sul client
    std::unordered_map<ENetPeer*, Player> players;

    // Conto alla rovescia per il cambio livello.
    uint32_t all_finished_time = 0;                       // enet_time quando tutti sono finished
    static constexpr uint32_t NEXT_LEVEL_DELAY_MS = 3000; // 3 secondi

    // Ricalcola se tutti i player sono finished e aggiorna all_finished_time.
    auto recheck_all_finished = [&]() {
        if (players.empty()) { all_finished_time = 0; return; }
        for (auto& [peer, pl] : players)
            if (!pl.GetState().finished) { all_finished_time = 0; return; }
        if (all_finished_time == 0)
            all_finished_time = enet_time_get();
    };

    ENetEvent event;
    while (!stop_flag) {
        while (enet_host_service(server, &event, 16) > 0) {
            switch (event.type) {

            case ENET_EVENT_TYPE_CONNECT: {
                PlayerState ps{};
                ps.player_id = next_player_id++;
                ps.x = spawn_x;
                ps.y = spawn_y;
                Player pl{};
                pl.SetState(ps);
                players[event.peer] = pl;

                // Informa il client del suo player_id.
                PktWelcome welcome{};
                welcome.player_id = ps.player_id;
                ENetPacket* wlc = enet_packet_create(&welcome, sizeof(welcome),
                                                     ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(event.peer, CHANNEL_RELIABLE, wlc);
                printf("[server] CONNECT player_id=%u  %08x:%u\n",
                       ps.player_id,
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

                            // Timer e rilevamento endpoint.
                            PlayerState s = it->second.GetState();
                            if (!s.finished) {
                                s.level_ticks++;
                                // Controlla se uno degli angoli del player è sul tile 'E'.
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
                                }
                                it->second.SetState(s);
                            }
                            recheck_all_finished();

                            // Costruisce il GameState con tutti i player e lo
                            // trasmette in broadcast a tutti i peer connessi.
                            PktGameState gs_pkt{};
                            gs_pkt.state.count = 0;
                            for (auto& [p, pl] : players) {
                                if (gs_pkt.state.count < static_cast<uint32_t>(MAX_PLAYERS))
                                    gs_pkt.state.players[gs_pkt.state.count++] = pl.GetState();
                            }
                            // Propaga il conto alla rovescia ai client.
                            if (all_finished_time != 0 && !players.empty()) {
                                const uint32_t el = enet_time_get() - all_finished_time;
                                if (el < NEXT_LEVEL_DELAY_MS)
                                    gs_pkt.state.next_level_countdown_ticks =
                                        (NEXT_LEVEL_DELAY_MS - el) * 60 / 1000;
                            }
                            ENetPacket* bcast = enet_packet_create(
                                &gs_pkt, sizeof(gs_pkt), 0);
                            enet_host_broadcast(server, CHANNEL_RELIABLE, bcast);
                            enet_host_flush(server);
                        }
                    }
                    // Aggiornamento nome player.
                    else if (type == PKT_PLAYER_INFO && len >= sizeof(PktPlayerInfo)) {
                        PktPlayerInfo info{};
                        std::memcpy(&info, event.packet->data, sizeof(PktPlayerInfo));
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
                }
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("[server] DISCONNECT %08x:%u\n",
                       event.peer->address.host,
                       event.peer->address.port);
                players.erase(event.peer);
                recheck_all_finished();
                break;

            default:
                break;
            }
        }

        // --- Controlla se è ora di passare al livello successivo ---
        if (all_finished_time != 0 && !players.empty() &&
            enet_time_get() - all_finished_time >= NEXT_LEVEL_DELAY_MS) {
            current_level++;
            char next_path[128];
            std::snprintf(next_path, sizeof(next_path),
                          "assets/levels/level_%02d.txt", current_level);
            PktLoadLevel ll_pkt{};
            World next_world;
            if (next_world.LoadFromFile(next_path)) {
                world = next_world;
                spawn_x = 0.f; spawn_y = 0.f;
                for (int ty = 0; ty < world.GetHeight(); ty++)
                    for (int tx = 0; tx < world.GetWidth(); tx++)
                        if (world.GetTile(tx, ty) == 'X') {
                            spawn_x = static_cast<float>(tx * TILE_SIZE);
                            spawn_y = static_cast<float>(ty * TILE_SIZE);
                        }
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
                printf("[server] LEVEL CHANGE \u2192 %s\n", next_path);
            } else {
                ll_pkt.is_last = 1;
                printf("[server] all levels complete, game over\n");
            }
            ENetPacket* ll = enet_packet_create(&ll_pkt, sizeof(ll_pkt),
                                                ENET_PACKET_FLAG_RELIABLE);
            enet_host_broadcast(server, CHANNEL_RELIABLE, ll);
            enet_host_flush(server);
            all_finished_time = 0;
            if (ll_pkt.is_last) stop_flag = true;
        }
    }

    enet_host_destroy(server);
    printf("[server] fermato\n");
}
