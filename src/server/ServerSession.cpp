// ServerSession.cpp — implementazione della macchina a stati della sessione.

#include "ServerSession.h"
#include "PlayerReset.h"  // SpawnReset, CheckpointReset
#include "SpawnFinder.h"   // FindCenterCheckpoint
#include "Physics.h"      // TILE_SIZE, FIXED_DT
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

// ---------------------------------------------------------------------------
// Costruttore
// ---------------------------------------------------------------------------
ServerSession::ServerSession(const char* initial_map_path, int initial_level, bool skip_lobby)
    : initial_map_path_(initial_map_path ? initial_map_path : "")
    , initial_level_(initial_level)
    , current_level_([&]{ return (std::strstr(initial_map_path, "_Lobby") ||
                                  std::strstr(initial_map_path, "_lobby"))
                                  ? 0 : initial_level_; }())
    , skip_lobby_(skip_lobby)
    , in_lobby_(!skip_lobby && (std::strstr(initial_map_path, "_Lobby") ||
                                std::strstr(initial_map_path, "_lobby")))
{
    // Load all chunks from the chunks directory for procedural generation.
    if (!chunk_store_.LoadFromDirectory("assets/levels/chunks")) {
        printf("[server] WARNING: no chunks loaded — level generation disabled\n");
    }

    // In skip_lobby mode, generate the first level immediately.
    // game_locked_ is set later in OnConnect (after the local client connects).
    if (skip_lobby_ && chunk_store_.IsReady()) {
        current_level_ = 1;
        is_ready_      = level_mgr_.Generate(current_level_, chunk_store_);
        if (is_ready_)
            printf("[server] skip_lobby: generated level 1 immediately\n");
    } else {
        is_ready_ = level_mgr_.Load(initial_map_path);
    }
    level_start_ms_ = enet_time_get();
}

// ---------------------------------------------------------------------------
// OnConnect
// ---------------------------------------------------------------------------
void ServerSession::OnConnect(ENetHost* host, ENetPeer* peer) {
    (void)host;
    if (game_locked_) {
        printf("[server] CONNECT rifiutato (partita in corso) %08x:%u\n",
               peer->address.host, peer->address.port);
        enet_peer_disconnect(peer, DISCONNECT_SERVER_BUSY);
        return;
    }

    if (session_token_ == 0u)
        session_token_ = enet_time_get() ^
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(peer) & 0xFFFFFFFFu);

    PlayerState ps{};
    ps.player_id = next_player_id_++;
    ps.x = level_mgr_.SpawnX();
    ps.y = level_mgr_.SpawnY();
    Player pl{};
    pl.SetState(ps);
    players_[peer] = pl;

    PktWelcome welcome{};
    welcome.player_id     = ps.player_id;
    welcome.session_token = session_token_;
    ENetPacket* wlc = enet_packet_create(&welcome, sizeof(welcome),
                                          ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, CHANNEL_RELIABLE, wlc);
    printf("[server] CONNECT player_id=%u  session=%u  %08x:%u\n",
           ps.player_id, session_token_,
           peer->address.host, peer->address.port);

    // In skip_lobby mode the level is already generated; lock the game and send it.
    if (skip_lobby_ && !in_lobby_) {
        game_locked_ = true;
        SendLevelDataToPeer(peer);
    }
}

// ---------------------------------------------------------------------------
// OnDisconnect
// ---------------------------------------------------------------------------
bool ServerSession::OnDisconnect(ENetHost* host, ENetPeer* peer) {
    printf("[server] DISCONNECT %08x:%u\n",
           peer->address.host, peer->address.port);
    players_.erase(peer);
    best_ticks_.erase(peer);
    ready_peers_.erase(peer);

    // Se in results/global_results e tutti i rimanenti già pronti → transizione immediata.
    if ((in_results_ || in_global_results_) && !players_.empty() &&
        ready_peers_.size() >= players_.size()) {
        if (in_global_results_) {
            in_global_results_ = false;
            ready_peers_.clear();
            FinishSession(host);
        } else {
            in_results_ = false;
            ready_peers_.clear();
            DoLevelChange(host);
        }
        return true;
    }

    // Nessun player rimasto → reset completo alla mappa iniziale.
    if (players_.empty()) {
        in_results_    = false;
        ready_peers_.clear();
        best_ticks_.clear();
        in_lobby_      = (initial_map_path_.find("_Lobby") != std::string::npos ||
                           initial_map_path_.find("_lobby") != std::string::npos);
        game_locked_   = false;
        session_token_ = 0u;
        current_level_ = in_lobby_ ? 0 : initial_level_;
        level_mgr_.Load(initial_map_path_.c_str());
        zone_start_ms_  = 0;
        level_start_ms_ = enet_time_get();
        next_player_id_ = 1;
        printf("[server] tutti disconnessi --> reset a '%s'\n",
               initial_map_path_.c_str());
    }
    return false;
}

// ---------------------------------------------------------------------------
// OnReceive — dispatch per tipo di pacchetto
// ---------------------------------------------------------------------------
bool ServerSession::OnReceive(ENetHost* host, ENetPeer* peer,
                               const uint8_t* data, size_t len) {
    if (len < 1) return false;
    const uint8_t type = data[0];

    if (type == PKT_INPUT && len >= sizeof(PktInput)) {
        PktInput pkt{};
        std::memcpy(&pkt, data, sizeof(PktInput));
        return HandleInput(host, peer, pkt);
    }
    if (type == PKT_PLAYER_INFO && len >= sizeof(PktPlayerInfo)) {
        PktPlayerInfo pkt{};
        std::memcpy(&pkt, data, sizeof(PktPlayerInfo));
        HandlePlayerInfo(host, peer, pkt);
        return false;
    }
    if (type == PKT_RESTART && len >= sizeof(PktRestart)) {
        HandleRestart(peer);
        return false;
    }
    if (type == PKT_RESTART_SPAWN && len >= sizeof(PktRestartSpawn)) {
        HandleRestartSpawn(peer);
        return false;
    }
    if (type == PKT_READY && (in_results_ || in_global_results_)) {
        return HandleReady(host, peer);
    }
    if (type == PKT_EMOTE && len >= sizeof(PktEmote)) {
        PktEmote epkt{};
        std::memcpy(&epkt, data, sizeof(PktEmote));
        // Relay to all clients as PKT_EMOTE_BROADCAST
        auto it = players_.find(peer);
        if (it != players_.end()) {
            PktEmoteBroadcast bcast{};
            bcast.emote_id  = epkt.emote_id;
            bcast.player_id = it->second.GetState().player_id;
            ENetPacket* pkt = enet_packet_create(&bcast, sizeof(bcast),
                                                  ENET_PACKET_FLAG_RELIABLE);
            enet_host_broadcast(host, CHANNEL_RELIABLE, pkt);
            enet_host_flush(host);
        }
        return false;
    }
    return false;
}

// ---------------------------------------------------------------------------
// CheckTimers — timer results (chiamato una volta per ciclo loop esterno)
// ---------------------------------------------------------------------------
void ServerSession::CheckTimers(ENetHost* host) {
    if (in_results_ && !players_.empty() &&
        enet_time_get() - results_start_ms_ >= RESULTS_DURATION_MS) {
        in_results_ = false;
        ready_peers_.clear();
        DoLevelChange(host);
    }
    if (in_global_results_ && !players_.empty() &&
        enet_time_get() - global_results_start_ms_ >= GLOBAL_RESULTS_DURATION_MS) {
        in_global_results_ = false;
        ready_peers_.clear();
        FinishSession(host);
    }
}

// ---------------------------------------------------------------------------
// HandleInput — simulazione fisica + finish/kill + broadcast + trigger cambio
// ---------------------------------------------------------------------------
bool ServerSession::HandleInput(ENetHost* host, ENetPeer* peer,
                                 const PktInput& pkt) {
    auto it = players_.find(peer);
    if (it == players_.end()) return false;

    const World& world = level_mgr_.GetWorld();
    it->second.Simulate(pkt.frame, world);

    PlayerState s = it->second.GetState();

    // --- Timer di livello + rilevamento tile 'E' (finish) ---
    if (!s.finished && s.kill_respawn_ticks == 0 && s.respawn_grace_ticks == 0) {
        s.level_ticks++;
        const int tx0 = static_cast<int>(s.x)                    / TILE_SIZE;
        const int ty0 = static_cast<int>(s.y)                    / TILE_SIZE;
        const int tx1 = static_cast<int>(s.x + TILE_SIZE - 1.f)  / TILE_SIZE;
        const int ty1 = static_cast<int>(s.y + TILE_SIZE - 1.f)  / TILE_SIZE;
        if (world.GetTile(tx0, ty0) == 'E' || world.GetTile(tx1, ty0) == 'E' ||
            world.GetTile(tx0, ty1) == 'E' || world.GetTile(tx1, ty1) == 'E') {
            s.finished = true;
            printf("[server] FINISH player_id=%u ticks=%u\n",
                   s.player_id, s.level_ticks);
            uint32_t& best = best_ticks_[peer];
            if (best == 0 || s.level_ticks < best) best = s.level_ticks;
        }
    }

    // --- Checkpoint tile 'C' ---
    if (!s.finished) {
        const int txc0 = static_cast<int>(s.x)                    / TILE_SIZE;
        const int tyc0 = static_cast<int>(s.y)                    / TILE_SIZE;
        const int txc1 = static_cast<int>(s.x + TILE_SIZE - 1.f)  / TILE_SIZE;
        const int tyc1 = static_cast<int>(s.y + TILE_SIZE - 1.f)  / TILE_SIZE;
        // Find first overlapping 'C' tile to use as flood-fill seed.
        int seed_tx = -1, seed_ty = -1;
        if      (world.GetTile(txc0, tyc0) == 'C') { seed_tx = txc0; seed_ty = tyc0; }
        else if (world.GetTile(txc1, tyc0) == 'C') { seed_tx = txc1; seed_ty = tyc0; }
        else if (world.GetTile(txc0, tyc1) == 'C') { seed_tx = txc0; seed_ty = tyc1; }
        else if (world.GetTile(txc1, tyc1) == 'C') { seed_tx = txc1; seed_ty = tyc1; }
        if (seed_tx >= 0) {
            // Use the same center+lowest-floor logic as the spawn finder,
            // applied to the full connected group of 'C' tiles.
            const SpawnPos cp = FindCenterCheckpoint(world, seed_tx, seed_ty);
            if (cp.x != s.checkpoint_x || cp.y != s.checkpoint_y) {
                s.checkpoint_x = cp.x;
                s.checkpoint_y = cp.y;
                printf("[server] CHECKPOINT player_id=%u  (%.0f, %.0f)\n",
                       s.player_id, cp.x, cp.y);
            }
        }
    }

    // --- Kill tile 'K' ---
    {
        const int tx0k = static_cast<int>(s.x)                    / TILE_SIZE;
        const int ty0k = static_cast<int>(s.y)                    / TILE_SIZE;
        const int tx1k = static_cast<int>(s.x + TILE_SIZE - 1.f)  / TILE_SIZE;
        const int ty1k = static_cast<int>(s.y + TILE_SIZE - 1.f)  / TILE_SIZE;
        if (world.GetTile(tx0k, ty0k) == 'K' || world.GetTile(tx1k, ty0k) == 'K' ||
            world.GetTile(tx0k, ty1k) == 'K' || world.GetTile(tx1k, ty1k) == 'K') {
            // Respawn at last checkpoint if available, otherwise at spawn.
            if (s.checkpoint_x != 0.f || s.checkpoint_y != 0.f)
                s = CheckpointReset(s, s.checkpoint_x, s.checkpoint_y, true);
            else
                s = ApplySpawnReset(s, true);
            printf("[server] KILL player_id=%u --> respawn in 1s\n", s.player_id);
        }
    }

    it->second.SetState(s);

    UpdateZone();
    BroadcastGameState(host);

    // --- Verifica scadenza timer zona ---
    if (zone_start_ms_ != 0 && !players_.empty() &&
        enet_time_get() - zone_start_ms_ >= NEXT_LEVEL_MS) {
        zone_start_ms_ = 0;
        if (in_lobby_) {
            DoLevelChange(host);
            return true;
        }
        if (!in_results_) {
            SendResults(host, "zona");
        }
    }

    // --- Verifica scadenza time limit (2 min) ---
    if (!in_lobby_ && !in_results_ && !players_.empty() &&
        enet_time_get() - level_start_ms_ >= LEVEL_TIME_LIMIT_MS) {
        zone_start_ms_ = 0;
        SendResults(host, "timeout");
    }
    return false;
}

// ---------------------------------------------------------------------------
// HandlePlayerInfo — aggiorna nome + controllo protocollo
// ---------------------------------------------------------------------------
void ServerSession::HandlePlayerInfo(ENetHost* /*host*/, ENetPeer* peer,
                                      const PktPlayerInfo& info) {
    if (info.protocol_version != PROTOCOL_VERSION) {
        printf("[server] VERSION MISMATCH peer=%08x client=%u server=%u --> disconnesso\n",
               peer->address.host, info.protocol_version, PROTOCOL_VERSION);
        PktVersionMismatch vm{};
        ENetPacket* vmpkt = enet_packet_create(&vm, sizeof(vm),
                                                ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, CHANNEL_RELIABLE, vmpkt);
        enet_peer_disconnect(peer, DISCONNECT_VERSION_MISMATCH);
        players_.erase(peer);
        return;
    }
    auto it = players_.find(peer);
    if (it != players_.end()) {
        PlayerState s = it->second.GetState();
        std::strncpy(s.name, info.name, sizeof(s.name) - 1);
        s.name[sizeof(s.name) - 1] = '\0';
        session_names_[s.player_id] = s.name;
        it->second.SetState(s);
        printf("[server] PLAYER_INFO id=%u name='%s'\n", s.player_id, s.name);
    }
}

// ---------------------------------------------------------------------------
// HandleRestart — ripartenza dal checkpoint (o spawn se nessun checkpoint raggiunto)
// ---------------------------------------------------------------------------
void ServerSession::HandleRestart(ENetPeer* peer) {
    auto it = players_.find(peer);
    if (it == players_.end()) return;
    PlayerState s = it->second.GetState();
    if (s.kill_respawn_ticks > 0) return;  // morto, ignora
    if (s.checkpoint_x != 0.f || s.checkpoint_y != 0.f)
        s = CheckpointReset(s, s.checkpoint_x, s.checkpoint_y, false);
    else
        s = ApplySpawnReset(s, false);
    it->second.SetState(s);
    printf("[server] RESTART(checkpoint) player_id=%u  (%.0f, %.0f)\n",
           s.player_id, s.checkpoint_x, s.checkpoint_y);
}

// ---------------------------------------------------------------------------
// HandleRestartSpawn — ripartenza forzata dallo spawn (ignora checkpoint)
// ---------------------------------------------------------------------------
void ServerSession::HandleRestartSpawn(ENetPeer* peer) {
    auto it = players_.find(peer);
    if (it == players_.end()) return;
    PlayerState s = it->second.GetState();
    if (s.kill_respawn_ticks > 0) return;  // morto, ignora
    s = ApplySpawnReset(s, false);  // clears checkpoint too
    it->second.SetState(s);
    printf("[server] RESTART(spawn) player_id=%u\n", s.player_id);
}

// ---------------------------------------------------------------------------
// HandleReady — giocatore pronto durante la fase results
// ---------------------------------------------------------------------------
bool ServerSession::HandleReady(ENetHost* host, ENetPeer* peer) {
    ready_peers_.insert(peer);
    const auto it2 = players_.find(peer);
    const uint32_t pid = (it2 != players_.end())
        ? it2->second.GetState().player_id : 0u;
    printf("[server] READY player_id=%u  (%zu/%zu)\n",
           pid, ready_peers_.size(), players_.size());
    if (ready_peers_.size() >= players_.size()) {
        if (in_global_results_) {
            in_global_results_ = false;
            ready_peers_.clear();
            FinishSession(host);
        } else {
            in_results_ = false;
            ready_peers_.clear();
            DoLevelChange(host);
        }
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// DoLevelChange — transizione al livello successivo (privato)
// ---------------------------------------------------------------------------
void ServerSession::DoLevelChange(ENetHost* host) {
    in_results_ = false;
    ready_peers_.clear();
    best_ticks_.clear();

    // Lobby → primo livello reale: blocca nuove connessioni per questa sessione.
    if (in_lobby_) {
        in_lobby_     = false;
        game_locked_  = true;
        current_level_ = 1;
        printf("[server] LOBBY COMPLETE --> GAME  session_token=%u\n", session_token_);
    } else {
        current_level_++;
    }

    // Check if session is over (max generated levels reached).
    if (current_level_ > MAX_GENERATED_LEVELS) {
        printf("[server] all levels complete --> global results\n");
        zone_start_ms_ = 0;
        SendGlobalResults(host);
        return;
    }

    // Generate level from chunks (or fall back to file-based loading).
    bool loaded = false;
    if (chunk_store_.IsReady()) {
        loaded = level_mgr_.Generate(current_level_, chunk_store_);
    }
    if (!loaded) {
        // Fallback: try file-based loading (legacy path).
        const std::string next_path = LevelManager::BuildPath(current_level_);
        loaded = level_mgr_.Load(next_path.c_str());
    }

    if (loaded) {
        for (auto& [peer, pl] : players_) {
            PlayerState s = pl.GetState();
            s.x = level_mgr_.SpawnX();  s.y = level_mgr_.SpawnY();
            s.vel_x = 0.f;  s.vel_y = 0.f;
            s.move_vel_x  = 0.f;
            s.level_ticks = 0;
            s.finished    = false;
            s.kill_respawn_ticks  = 0;
            s.respawn_grace_ticks = 25;
            s.checkpoint_x = 0.f;  // clear checkpoint on level change
            s.checkpoint_y = 0.f;
            pl.SetState(s);
        }

        // Send the generated level data to all clients.
        BroadcastLevelData(host);
        printf("[server] LEVEL CHANGE --> generated level %d\n", current_level_);

        zone_start_ms_  = 0;
        level_start_ms_ = enet_time_get();
    } else {
        // Nessun livello successivo trovato → mostra classifica globale prima di chiudere.
        printf("[server] all levels complete --> global results\n");
        zone_start_ms_ = 0;
        SendGlobalResults(host);
    }
}

// ---------------------------------------------------------------------------
// ResetToInitial — disconnette tutti e ricarica la lobby
// ---------------------------------------------------------------------------
void ServerSession::ResetToInitial(ENetHost* host) {
    (void)host;
    for (auto& [peer, pl] : players_)
        enet_peer_disconnect_now(peer, 0);
    players_.clear();

    session_wins_.clear();
    session_names_.clear();
    in_global_results_ = false;

    in_lobby_      = (initial_map_path_.find("_Lobby") != std::string::npos ||
                       initial_map_path_.find("_lobby") != std::string::npos);
    game_locked_   = false;
    session_token_ = 0u;
    current_level_ = in_lobby_ ? 0 : initial_level_;
    level_mgr_.Load(initial_map_path_.c_str());
    level_start_ms_ = enet_time_get();
    printf("[server] reset completato, lobby riaperta\n");
}

// ---------------------------------------------------------------------------
// SendResults — costruisce e broadcast PktLevelResults
// ---------------------------------------------------------------------------
void ServerSession::SendResults(ENetHost* host, const char* reason) {
    PktLevelResults res_pkt{};
    res_pkt.level = static_cast<uint8_t>(current_level_);

    std::vector<ResultEntry> entries;
    for (auto& [peer, pl] : players_) {
        const PlayerState& s = pl.GetState();
        ResultEntry e{};
        e.player_id = s.player_id;
        std::strncpy(e.name, s.name, sizeof(e.name) - 1);
        const auto best_it = best_ticks_.find(peer);
        if (best_it != best_ticks_.end() && best_it->second > 0) {
            e.finished    = 1u;
            e.level_ticks = best_it->second;
        } else {
            e.finished    = 0u;
            e.level_ticks = s.level_ticks;
        }
        entries.push_back(e);
    }
    std::sort(entries.begin(), entries.end(),
        [](const ResultEntry& a, const ResultEntry& b) {
            if (a.finished != b.finished) return a.finished > b.finished;
            return a.level_ticks < b.level_ticks;
        });
    // Registra il vincitore del livello (1° classificato tra chi ha finito).
    if (!entries.empty() && entries[0].finished) {
        session_wins_[entries[0].player_id]++;
        printf("[server] WIN player_id=%u (total wins=%u)\n",
               entries[0].player_id, session_wins_[entries[0].player_id]);
    }

    res_pkt.count = static_cast<uint8_t>(entries.size());
    for (size_t i = 0; i < entries.size() && i < static_cast<size_t>(MAX_PLAYERS); i++)
        res_pkt.entries[i] = entries[i];

    ENetPacket* resl = enet_packet_create(&res_pkt, sizeof(res_pkt),
                                           ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(host, CHANNEL_RELIABLE, resl);
    enet_host_flush(host);

    in_results_       = true;
    results_start_ms_ = enet_time_get();
    ready_peers_.clear();
    printf("[server] RESULTS (%s) level=%d players=%zu\n",
           reason, current_level_, entries.size());
}

// ---------------------------------------------------------------------------
// BroadcastGameState
// ---------------------------------------------------------------------------
void ServerSession::BroadcastGameState(ENetHost* host) {
    PktGameState gs_pkt{};
    gs_pkt.state.count = 0;
    for (auto& [peer, pl] : players_) {
        if (gs_pkt.state.count < static_cast<uint32_t>(MAX_PLAYERS))
            gs_pkt.state.players[gs_pkt.state.count++] = pl.GetState();
    }
    gs_pkt.state.next_level_countdown_ticks = CountdownTicks();
    gs_pkt.state.is_lobby = in_lobby_ ? 1u : 0u;
    if (!in_lobby_ && !in_results_) {
        const uint32_t el = enet_time_get() - level_start_ms_;
        gs_pkt.state.time_limit_secs = el < LEVEL_TIME_LIMIT_MS
            ? (LEVEL_TIME_LIMIT_MS - el) / 1000u : 0u;
    }
    ENetPacket* bcast = enet_packet_create(&gs_pkt, sizeof(gs_pkt), 0);
    enet_host_broadcast(host, CHANNEL_RELIABLE, bcast);
    enet_host_flush(host);
}

// ---------------------------------------------------------------------------
// BroadcastLevelData — send PKT_LEVEL_DATA with the current world grid
// ---------------------------------------------------------------------------
void ServerSession::BroadcastLevelData(ENetHost* host) {
    const World& world = level_mgr_.GetWorld();
    const int w = world.GetWidth();
    const int h = world.GetHeight();
    const size_t grid_size = static_cast<size_t>(w) * h;
    const size_t pkt_size  = sizeof(PktLevelDataHeader) + grid_size;

    // Build packet buffer: header + tile char data
    std::vector<uint8_t> buf(pkt_size, 0);
    PktLevelDataHeader hdr{};
    hdr.type    = PKT_LEVEL_DATA;
    hdr.is_last = 0;
    hdr.width   = static_cast<uint16_t>(w);
    hdr.height  = static_cast<uint16_t>(h);
    hdr.level   = static_cast<uint8_t>(current_level_);
    std::memcpy(buf.data(), &hdr, sizeof(hdr));

    // Copy tile chars row-by-row
    const auto& rows = world.GetRows();
    uint8_t* dst = buf.data() + sizeof(hdr);
    for (int y = 0; y < h; ++y) {
        std::memcpy(dst, rows[y].data(), w);
        dst += w;
    }

    ENetPacket* pkt = enet_packet_create(buf.data(), pkt_size,
                                          ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(host, CHANNEL_RELIABLE, pkt);
    enet_host_flush(host);
    printf("[server] PKT_LEVEL_DATA sent: %dx%d = %zu bytes\n", w, h, pkt_size);
}

// ---------------------------------------------------------------------------
// SendLevelDataToPeer — send PKT_LEVEL_DATA to a single peer (used on connect)
// ---------------------------------------------------------------------------
void ServerSession::SendLevelDataToPeer(ENetPeer* peer) {
    const World& world = level_mgr_.GetWorld();
    const int w = world.GetWidth();
    const int h = world.GetHeight();
    if (w == 0 || h == 0) return;
    const size_t grid_size = static_cast<size_t>(w) * h;
    const size_t pkt_size  = sizeof(PktLevelDataHeader) + grid_size;

    std::vector<uint8_t> buf(pkt_size, 0);
    PktLevelDataHeader hdr{};
    hdr.type    = PKT_LEVEL_DATA;
    hdr.is_last = 0;
    hdr.width   = static_cast<uint16_t>(w);
    hdr.height  = static_cast<uint16_t>(h);
    hdr.level   = static_cast<uint8_t>(current_level_);
    std::memcpy(buf.data(), &hdr, sizeof(hdr));

    const auto& rows = world.GetRows();
    uint8_t* dst = buf.data() + sizeof(hdr);
    for (int y = 0; y < h; ++y) {
        std::memcpy(dst, rows[y].data(), w);
        dst += w;
    }

    ENetPacket* pkt = enet_packet_create(buf.data(), pkt_size,
                                          ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, CHANNEL_RELIABLE, pkt);
    printf("[server] PKT_LEVEL_DATA sent to peer: %dx%d = %zu bytes\n", w, h, pkt_size);
}

// ---------------------------------------------------------------------------
// UpdateZone
// ---------------------------------------------------------------------------
void ServerSession::UpdateZone() {
    if (AllInZone()) {
        if (zone_start_ms_ == 0)
            zone_start_ms_ = enet_time_get();
    } else {
        zone_start_ms_ = 0;
    }
}

// ---------------------------------------------------------------------------
// AllInZone
// ---------------------------------------------------------------------------
bool ServerSession::AllInZone() const {
    if (players_.empty()) return false;
    const World& world = level_mgr_.GetWorld();
    for (const auto& [peer, pl] : players_) {
        const PlayerState& s = pl.GetState();
        const int tx0 = static_cast<int>(s.x)                    / TILE_SIZE;
        const int ty0 = static_cast<int>(s.y)                    / TILE_SIZE;
        const int tx1 = static_cast<int>(s.x + TILE_SIZE - 1.f)  / TILE_SIZE;
        const int ty1 = static_cast<int>(s.y + TILE_SIZE - 1.f)  / TILE_SIZE;
        const bool on_e = world.GetTile(tx0, ty0) == 'E' ||
                          world.GetTile(tx1, ty0) == 'E' ||
                          world.GetTile(tx0, ty1) == 'E' ||
                          world.GetTile(tx1, ty1) == 'E';
        if (!on_e) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// CountdownTicks
// ---------------------------------------------------------------------------
uint32_t ServerSession::CountdownTicks() const {
    if (zone_start_ms_ == 0) return 0;
    const uint32_t elapsed = enet_time_get() - zone_start_ms_;
    if (elapsed >= NEXT_LEVEL_MS) return 0;
    return (NEXT_LEVEL_MS - elapsed) * 60u / 1000u;
}

// ---------------------------------------------------------------------------
// ApplySpawnReset
// ---------------------------------------------------------------------------
PlayerState ServerSession::ApplySpawnReset(PlayerState s, bool with_kill) const {
    return SpawnReset(s, level_mgr_.SpawnX(), level_mgr_.SpawnY(), with_kill);
}

// ---------------------------------------------------------------------------
// SendGlobalResults — classifica vittorie di sessione
// ---------------------------------------------------------------------------
void ServerSession::SendGlobalResults(ENetHost* host) {
    // Unisci giocatori connessi + dati di chi ha già disconnesso (in session_wins_)
    std::unordered_map<uint32_t, uint32_t> all_wins;
    for (const auto& [peer, pl] : players_) {
        const uint32_t id = pl.GetState().player_id;
        all_wins[id] = session_wins_.count(id) ? session_wins_.at(id) : 0u;
    }
    for (const auto& [id, w] : session_wins_)
        if (!all_wins.count(id)) all_wins[id] = w;

    std::vector<GlobalResultEntry> entries;
    entries.reserve(all_wins.size());
    for (const auto& [id, wins] : all_wins) {
        GlobalResultEntry e{};
        e.player_id = id;
        e.wins      = wins;
        const auto ni = session_names_.find(id);
        if (ni != session_names_.end())
            std::strncpy(e.name, ni->second.c_str(), sizeof(e.name) - 1);
        entries.push_back(e);
    }
    std::sort(entries.begin(), entries.end(),
        [](const GlobalResultEntry& a, const GlobalResultEntry& b) {
            return a.wins > b.wins;
        });

    PktGlobalResults pkt{};
    pkt.total_levels = static_cast<uint8_t>(current_level_ - 1);  // current_level_ was incremented to the failed load
    pkt.count = static_cast<uint8_t>(
        std::min(entries.size(), static_cast<size_t>(MAX_PLAYERS)));
    for (size_t i = 0; i < pkt.count; i++)
        pkt.entries[i] = entries[i];

    ENetPacket* p = enet_packet_create(&pkt, sizeof(pkt),
                                        ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(host, CHANNEL_RELIABLE, p);
    enet_host_flush(host);

    in_global_results_        = true;
    global_results_start_ms_  = enet_time_get();
    ready_peers_.clear();
    printf("[server] GLOBAL RESULTS: %d livelli, %zu giocatori\n",
           current_level_, entries.size());
}

// ---------------------------------------------------------------------------
// FinishSession — termina la sessione dopo la schermata globale
// ---------------------------------------------------------------------------
void ServerSession::FinishSession(ENetHost* host) {
    PktLoadLevel ll_pkt{};
    ll_pkt.is_last = 1;
    ENetPacket* ll = enet_packet_create(&ll_pkt, sizeof(ll_pkt),
                                         ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(host, CHANNEL_RELIABLE, ll);
    enet_host_flush(host);
    printf("[server] SESSION COMPLETE --> reset\n");
    ResetToInitial(host);
}
