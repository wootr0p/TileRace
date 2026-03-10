// ServerSession.cpp — implementazione della macchina a stati della sessione.

#include "ServerSession.h"
#include "PlayerReset.h"  // SpawnReset, CheckpointReset
#include "SpawnFinder.h"   // FindCenterCheckpoint
#include "Physics.h"      // TILE_SIZE, FIXED_DT
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

// ---------------------------------------------------------------------------
// Costruttore
// ---------------------------------------------------------------------------
ServerSession::ServerSession(const char* initial_map_path, int initial_level,
                             bool skip_lobby, GameMode initial_mode)
    : initial_map_path_(initial_map_path ? initial_map_path : "")
    , initial_level_(initial_level)
    , current_level_([&]{ return (std::strstr(initial_map_path, "_Lobby") ||
                                  std::strstr(initial_map_path, "_lobby"))
                                  ? 0 : initial_level_; }())
    , skip_lobby_(skip_lobby)
    , in_lobby_(!skip_lobby && (std::strstr(initial_map_path, "_Lobby") ||
                                std::strstr(initial_map_path, "_lobby")))
    , game_mode_(initial_mode)
{
    // Load all chunks from the chunks directory for procedural generation.
    if (!chunk_store_.LoadFromDirectory("assets/levels/chunks")) {
        printf("[server] WARNING: no chunks loaded — level generation disabled\n");
    }

    // In skip_lobby mode, generate the first level immediately.
    // game_locked_ is set later in OnConnect (after the local client connects).
    if (skip_lobby_ && chunk_store_.IsReady()) {
        current_level_ = 1;
        is_ready_      = level_mgr_.Generate(current_level_, chunk_store_, 0, skip_lobby_);
        if (is_ready_) {
            // Strip checkpoints for race mode.
            if (game_mode_ == GameMode::RACE)
                level_mgr_.GetWorldMut().StripCheckpoints();
            printf("[server] skip_lobby: generated level 1 immediately (mode=%s)\n",
                   game_mode_ == GameMode::RACE ? "RACE" : "COOP");
        }
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
    if (skip_lobby_ && !in_lobby_) {
        ps = SpawnReset(ps, level_mgr_.SpawnX(), level_mgr_.SpawnY(), false);
    } else {
        ps.x = level_mgr_.SpawnX();
        ps.y = level_mgr_.SpawnY();
    }
    Player pl{};
    pl.SetState(ps);
    players_[peer] = pl;

    // Leader election: first connected player becomes the leader.
    if (leader_id_ == 0)
        leader_id_ = ps.player_id;

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

    // Release magnet grab relationships involving this peer.
    ReleaseGrab(peer);  // if this peer was a grabber, free the target
    // If this peer was being grabbed, remove the entry from the grabber.
    for (auto it = grab_targets_.begin(); it != grab_targets_.end(); ) {
        if (it->second == peer) { it = grab_targets_.erase(it); }
        else ++it;
    }

    players_.erase(peer);
    best_ticks_.erase(peer);
    ready_peers_.erase(peer);

    // Leader promotion: if the leader disconnected, elect a new one.
    ElectLeader();

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
        leader_id_     = 0;
        game_mode_     = GameMode::COOP;
        session_max_levels_ = static_cast<uint8_t>(MAX_GENERATED_LEVELS);
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
    if (type == PKT_SET_GAME_MODE && len >= sizeof(PktSetGameMode)) {
        PktSetGameMode mpkt{};
        std::memcpy(&mpkt, data, sizeof(PktSetGameMode));
        HandleSetGameMode(host, peer, mpkt);
        return false;
    }
    if (type == PKT_SET_MAX_LEVELS && len >= sizeof(PktSetMaxLevels)) {
        PktSetMaxLevels lpkt{};
        std::memcpy(&lpkt, data, sizeof(PktSetMaxLevels));
        HandleSetMaxLevels(host, peer, lpkt);
        return false;
    }
    if (type == PKT_START_GAME && len >= sizeof(PktStartGame)) {
        return HandleStartGame(host, peer);
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
    ENetPeer* break_free_peer = nullptr;

    // Co-op: if a grabbed player presses jump/dash, release before simulation so
    // the same input frame can immediately trigger jump or dash.
    if (game_mode_ == GameMode::COOP) {
        const PlayerState& pre = it->second.GetState();
        if (pre.grabbed && (pkt.frame.Has(BTN_JUMP_PRESS) || pkt.frame.Has(BTN_DASH))) {
            for (auto& [grabber, grabbed_peer] : grab_targets_) {
                if (grabbed_peer == peer) {
                    ReleaseGrab(grabber);
                    break_free_peer = peer;
                    break;
                }
            }
        }
    }

    // Co-op: if the grabber starts a new dash while holding a player, throw the grabbed
    // player in the direction of the dash, then release the grab.
    if (game_mode_ == GameMode::COOP && pkt.frame.Has(BTN_DASH)) {
        const PlayerState& pre = it->second.GetState();
        if (pre.dash_ready && pre.dash_cooldown_ticks == 0 && pre.dash_active_ticks == 0) {
            auto git = grab_targets_.find(peer);
            if (git != grab_targets_.end()) {
                ENetPeer* thrown_peer = git->second;
                auto tit = players_.find(thrown_peer);
                if (tit != players_.end()) {
                    // Compute normalised dash direction (mirrors RequestDash logic).
                    float ddx = pkt.frame.dash_dx;
                    float ddy = pkt.frame.dash_dy;
                    const float len2 = ddx * ddx + ddy * ddy;
                    if (len2 > 0.000001f) {
                        const float inv = 1.f / std::sqrt(len2);
                        ddx *= inv;
                        ddy *= inv;
                    } else {
                        ddx = 0.f;
                        ddy = -1.f;  // default: throw upward
                    }
                    // Apply throw impulse to the grabbed player.
                    PlayerState ts = tit->second.GetState();
                    ts.vel_x = ddx * DASH_SPEED;
                    ts.vel_y = ddy * DASH_SPEED;
                    tit->second.SetState(ts);
                }
                ReleaseGrab(peer);
                // Prevent the thrown player from being immediately re-grabbed this tick.
                if (break_free_peer == nullptr)
                    break_free_peer = thrown_peer;
            }
        }
    }

    // Finished players still run physics, but their gameplay input is ignored.
    // This keeps movement/collisions authoritative while preventing any new actions.
    InputFrame sim_frame = pkt.frame;
    if (it->second.GetState().finished) {
        sim_frame.buttons = 0;
        sim_frame.move_x  = 0.f;
        sim_frame.dash_dx = 0.f;
        sim_frame.dash_dy = 0.f;
    }
    it->second.Simulate(sim_frame, world);

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

    // --- Checkpoint tile 'C' (shared: activates for all players) — COOP only ---
    if (game_mode_ == GameMode::COOP && !s.finished) {
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
            const SpawnPos cp = FindCenterCheckpoint(world, seed_tx, seed_ty);
            // Check if this checkpoint was already activated.
            bool already = false;
            for (auto& [ax, ay] : activated_checkpoints_) {
                if (ax == cp.x && ay == cp.y) { already = true; break; }
            }
            if (!already) {
                activated_checkpoints_.push_back({cp.x, cp.y});
                // Propagate + reset from the new shared checkpoint for ALL players.
                for (auto& [p, player] : players_) {
                    PlayerState ps = player.GetState();
                    ps.checkpoint_x = cp.x;
                    ps.checkpoint_y = cp.y;
                    ps = CheckpointReset(ps, cp.x, cp.y, false);
                    if (p == peer) s = ps;      // will be SetState'd below
                    else           player.SetState(ps);
                }
                printf("[server] SHARED CHECKPOINT player_id=%u activated (%.0f, %.0f) → reset all players\n",
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
    // Apply magnet grab/carry and player collisions — coop mode only.
    if (game_mode_ == GameMode::COOP) {
        ApplyMagnetGrab(break_free_peer);
        ResolvePlayerCollisions(world);
    }
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
// HandleSetGameMode — leader changes the game mode (lobby only)
// ---------------------------------------------------------------------------
void ServerSession::HandleSetGameMode(ENetHost* host, ENetPeer* peer, const PktSetGameMode& pkt) {
    if (!in_lobby_) return;  // mode can only be changed in the lobby

    auto it = players_.find(peer);
    if (it == players_.end()) return;
    const uint32_t pid = it->second.GetState().player_id;
    if (pid != leader_id_) {
        printf("[server] SET_GAME_MODE rejected: player_id=%u is not leader (%u)\n",
               pid, leader_id_);
        return;
    }
    const uint8_t mode = pkt.game_mode;
    if (mode > static_cast<uint8_t>(GameMode::RACE)) return;  // invalid mode

    game_mode_ = static_cast<GameMode>(mode);
    printf("[server] GAME MODE changed to %s by leader %u\n",
           game_mode_ == GameMode::RACE ? "RACE" : "COOP", leader_id_);
    BroadcastGameState(host);
}

// ---------------------------------------------------------------------------
// HandleSetMaxLevels — leader changes generated level count (lobby only)
// ---------------------------------------------------------------------------
void ServerSession::HandleSetMaxLevels(ENetHost* host, ENetPeer* peer, const PktSetMaxLevels& pkt) {
    if (!in_lobby_) return;

    auto it = players_.find(peer);
    if (it == players_.end()) return;
    const uint32_t pid = it->second.GetState().player_id;
    if (pid != leader_id_) {
        printf("[server] SET_MAX_LEVELS rejected: player_id=%u is not leader (%u)\n",
               pid, leader_id_);
        return;
    }

    uint8_t clamped = pkt.max_levels;
    if (clamped < static_cast<uint8_t>(MIN_GENERATED_LEVELS))
        clamped = static_cast<uint8_t>(MIN_GENERATED_LEVELS);
    if (clamped > static_cast<uint8_t>(MAX_GENERATED_LEVELS_LIMIT))
        clamped = static_cast<uint8_t>(MAX_GENERATED_LEVELS_LIMIT);
    if (clamped == session_max_levels_) return;

    session_max_levels_ = clamped;
    printf("[server] MAX LEVELS changed to %u by leader %u\n",
           static_cast<unsigned>(session_max_levels_), leader_id_);
    BroadcastGameState(host);
}

// ---------------------------------------------------------------------------
// HandleStartGame — leader starts the game from lobby
// ---------------------------------------------------------------------------
bool ServerSession::HandleStartGame(ENetHost* host, ENetPeer* peer) {
    if (!in_lobby_) return false;

    auto it = players_.find(peer);
    if (it == players_.end()) return false;
    const uint32_t pid = it->second.GetState().player_id;
    if (pid != leader_id_) {
        printf("[server] START_GAME rejected: player_id=%u is not leader (%u)\n",
               pid, leader_id_);
        return false;
    }

    printf("[server] START_GAME by leader %u\n", leader_id_);
    zone_start_ms_ = 0;
    DoLevelChange(host);
    return true;
}

// ---------------------------------------------------------------------------
// ElectLeader — promote the player with the lowest player_id
// ---------------------------------------------------------------------------
void ServerSession::ElectLeader() {
    if (players_.empty()) {
        leader_id_ = 0;
        return;
    }
    // Check if the current leader is still connected.
    bool leader_alive = false;
    for (const auto& [peer, pl] : players_) {
        if (pl.GetState().player_id == leader_id_) {
            leader_alive = true;
            break;
        }
    }
    if (leader_alive) return;

    // Promote the player with the lowest player_id (earliest assigned).
    uint32_t best_id = UINT32_MAX;
    for (const auto& [peer, pl] : players_) {
        const uint32_t id = pl.GetState().player_id;
        if (id < best_id) best_id = id;
    }
    leader_id_ = best_id;
    printf("[server] LEADER promoted: player_id=%u\n", leader_id_);
}

// ---------------------------------------------------------------------------
// DoLevelChange — transizione al livello successivo (privato)
// ---------------------------------------------------------------------------
void ServerSession::DoLevelChange(ENetHost* host) {
    in_results_ = false;
    ready_peers_.clear();
    best_ticks_.clear();
    activated_checkpoints_.clear();
    grab_targets_.clear();
    regrab_requires_release_.clear();

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
    if (current_level_ > static_cast<int>(session_max_levels_)) {
        printf("[server] all levels complete --> global results\n");
        zone_start_ms_ = 0;
        SendGlobalResults(host);
        return;
    }

    // Generate level from chunks (or fall back to file-based loading).
    bool loaded = false;
    if (chunk_store_.IsReady()) {
        // Notify clients that generation is starting so they show a loading overlay
        // and don't time out while the ENet loop is stalled.
        BroadcastGenerating(host);
        // Co-op gets a steeper ramp: reach high difficulty earlier within the session.
        const int curve_levels = (game_mode_ == GameMode::COOP)
            ? static_cast<int>(session_max_levels_)
            : DIFFICULTY_CURVE_LEVELS;
        loaded = level_mgr_.Generate(current_level_, chunk_store_, 0, skip_lobby_, curve_levels);
    }
    if (!loaded) {
        // Fallback: try file-based loading (legacy path).
        const std::string next_path = LevelManager::BuildPath(current_level_);
        loaded = level_mgr_.Load(next_path.c_str());
    }

    if (loaded) {
        // Race mode: strip checkpoint tiles from the generated level.
        if (game_mode_ == GameMode::RACE)
            level_mgr_.GetWorldMut().StripCheckpoints();

        for (auto& [peer, pl] : players_) {
            // Full reset: clears dash/jump/movement state that was previously leaking
            // across levels (e.g. dash_active_ticks carrying over → instant dash on spawn).
            PlayerState s = ApplySpawnReset(pl.GetState(), false);
            pl.SetState(s);
            pl.ResetTransient();  // clear non-serialised edge-detection flags
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
    regrab_requires_release_.clear();
    in_global_results_ = false;

    in_lobby_      = (initial_map_path_.find("_Lobby") != std::string::npos ||
                       initial_map_path_.find("_lobby") != std::string::npos);
    game_locked_   = false;
    coop_cleared_levels_ = 0;
    session_token_ = 0u;
    leader_id_     = 0;
    game_mode_     = GameMode::COOP;
    session_max_levels_ = static_cast<uint8_t>(MAX_GENERATED_LEVELS);
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

    const bool any_done = std::any_of(entries.begin(), entries.end(),
                                      [](const ResultEntry& e){ return e.finished != 0; });
    // Co-op rule: one player at the exit clears the level for everyone.
    const bool coop_cleared = (game_mode_ == GameMode::COOP) ? any_done : false;
    if (coop_cleared) {
        for (auto& e : entries) e.finished = 1u;
    }

    if (coop_cleared) {
        coop_cleared_levels_++;
        printf("[server] COOP CLEARED (total=%u)\n", coop_cleared_levels_);
    }

    // Race mode scoring: award one session win to the first finisher of this level.
    if (game_mode_ == GameMode::RACE && any_done) {
        for (const auto& e : entries) {
            if (!e.finished) continue;
            session_wins_[e.player_id]++;
            printf("[server] RACE WIN level=%d player_id=%u total_wins=%u\n",
                   current_level_, e.player_id, session_wins_[e.player_id]);
            break;  // only first place gets the win
        }
    }

    res_pkt.coop_all_finished = coop_cleared ? 1u : 0u;

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
    gs_pkt.state.is_lobby    = in_lobby_ ? 1u : 0u;
    gs_pkt.state.game_mode   = static_cast<uint8_t>(game_mode_);
    gs_pkt.state.max_generated_levels = session_max_levels_;
    gs_pkt.state.leader_id   = leader_id_;
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
// BroadcastGenerating — notify clients that level generation is starting
// ---------------------------------------------------------------------------
void ServerSession::BroadcastGenerating(ENetHost* host) {
    PktGenerating pkt{};
    pkt.level = static_cast<uint8_t>(current_level_);
    ENetPacket* ep = enet_packet_create(&pkt, sizeof(pkt), ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(host, CHANNEL_RELIABLE, ep);
    enet_host_flush(host);  // send immediately before blocking in Generate()
    printf("[server] PKT_GENERATING level=%u\n", (unsigned)pkt.level);
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
    // Co-op: one player reaching the exit is enough to clear the level.
    if (game_mode_ == GameMode::COOP && !in_lobby_) {
        for (const auto& [peer, pl] : players_)
            if (pl.GetState().finished) return true;
        return false;
    }
    // Race/lobby fallback: completion requires all players in the zone.
    for (const auto& [peer, pl] : players_)
        if (!pl.GetState().finished) return false;
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
// ResolvePlayerCollisions — push overlapping player AABBs apart
// ---------------------------------------------------------------------------

// Snap a single player out of any solid tile it overlaps (min-penetration axis).
static void ClampToWorld(PlayerState& s, const World& world) {
    // Iterate until no solid overlap remains (max 4 passes for corner cases).
    for (int pass = 0; pass < 4; ++pass) {
        const int tx0 = static_cast<int>(s.x)                    / TILE_SIZE;
        const int ty0 = static_cast<int>(s.y)                    / TILE_SIZE;
        const int tx1 = static_cast<int>(s.x + TILE_SIZE - 1.f)  / TILE_SIZE;
        const int ty1 = static_cast<int>(s.y + TILE_SIZE - 1.f)  / TILE_SIZE;

        float best_ov = 1e9f;
        int   best_axis = -1;  // 0=left, 1=right, 2=up, 3=down
        for (int ty = ty0; ty <= ty1; ++ty) {
            for (int tx = tx0; tx <= tx1; ++tx) {
                if (!world.IsSolid(tx, ty)) continue;
                const float tile_l = static_cast<float>(tx * TILE_SIZE);
                const float tile_r = tile_l + TILE_SIZE;
                const float tile_t = static_cast<float>(ty * TILE_SIZE);
                const float tile_b = tile_t + TILE_SIZE;
                const float ov_l = (s.x + TILE_SIZE) - tile_l;  // push left
                const float ov_r = tile_r - s.x;                // push right
                const float ov_u = (s.y + TILE_SIZE) - tile_t;  // push up
                const float ov_d = tile_b - s.y;                // push down
                if (ov_l > 0 && ov_l < best_ov) { best_ov = ov_l; best_axis = 0; }
                if (ov_r > 0 && ov_r < best_ov) { best_ov = ov_r; best_axis = 1; }
                if (ov_u > 0 && ov_u < best_ov) { best_ov = ov_u; best_axis = 2; }
                if (ov_d > 0 && ov_d < best_ov) { best_ov = ov_d; best_axis = 3; }
            }
        }
        if (best_axis < 0) break;  // no overlap
        switch (best_axis) {
            case 0: s.x -= best_ov; if (s.vel_x > 0.f) s.vel_x = 0.f; break;
            case 1: s.x += best_ov; if (s.vel_x < 0.f) s.vel_x = 0.f; break;
            case 2: s.y -= best_ov; if (s.vel_y > 0.f) s.vel_y = 0.f; s.on_ground = true; break;
            case 3: s.y += best_ov; if (s.vel_y < 0.f) s.vel_y = 0.f; break;
        }
    }
}

// ---------------------------------------------------------------------------
// ReleaseGrab — release a grabbed player (if any) held by the given grabber
// ---------------------------------------------------------------------------
void ServerSession::ReleaseGrab(ENetPeer* grabber) {
    auto it = grab_targets_.find(grabber);
    if (it == grab_targets_.end()) return;
    ENetPeer* target = it->second;
    auto tgt = players_.find(target);
    if (tgt != players_.end()) {
        PlayerState s = tgt->second.GetState();
        s.grabbed = false;
        tgt->second.SetState(s);
    }
    grab_targets_.erase(it);
    // Require a fresh grab press before this grabber can grab again.
    regrab_requires_release_.insert(grabber);
}

// ---------------------------------------------------------------------------
// ApplyMagnetGrab — magnet holders grab the closest player and carry them
// ---------------------------------------------------------------------------
void ServerSession::ApplyMagnetGrab(ENetPeer* break_free) {
    if (players_.size() < 2) return;

    // Latch release: after any release, a grabber must let go of magnet first.
    for (auto it = regrab_requires_release_.begin(); it != regrab_requires_release_.end(); ) {
        auto pit = players_.find(*it);
        if (pit == players_.end()) {
            it = regrab_requires_release_.erase(it);
            continue;
        }
        const PlayerState& ps = pit->second.GetState();
        if (!ps.magneting || ps.kill_respawn_ticks > 0 || ps.respawn_grace_ticks > 0 || ps.finished)
            it = regrab_requires_release_.erase(it);
        else
            ++it;
    }

    // 1. Release grabs for players that stopped magneting, died, or whose target is gone/dead.
    std::vector<ENetPeer*> to_release;
    for (auto& [grabber, grabbed] : grab_targets_) {
        auto git = players_.find(grabber);
        auto tit = players_.find(grabbed);
        bool release = false;
        if (git == players_.end() || tit == players_.end()) release = true;
        else {
            const PlayerState& gs = git->second.GetState();
            const PlayerState& ts = tit->second.GetState();
            if (!gs.magneting) release = true;
            if (gs.kill_respawn_ticks > 0 || gs.respawn_grace_ticks > 0) release = true;
            if (gs.finished) release = true;
            if (ts.kill_respawn_ticks > 0 || ts.respawn_grace_ticks > 0) release = true;
            if (ts.finished) release = true;
        }
        if (release) to_release.push_back(grabber);
    }
    for (ENetPeer* g : to_release) ReleaseGrab(g);

    // 2. For each magneting player without a grab target, find the closest eligible player.
    for (auto& [peer, pl] : players_) {
        const PlayerState& ps = pl.GetState();
        if (!ps.magneting) continue;
        if (ps.kill_respawn_ticks > 0 || ps.respawn_grace_ticks > 0 || ps.finished) continue;
        if (grab_targets_.count(peer)) continue;  // already holding someone
        if (regrab_requires_release_.count(peer)) continue;  // still holding old press

        const float mx = ps.x + TILE_SIZE * 0.5f;
        const float my = ps.y + TILE_SIZE * 0.5f;
        float best_dist2 = MAGNET_RANGE * MAGNET_RANGE;
        ENetPeer* best = nullptr;

        for (auto& [other_peer, other_pl] : players_) {
            if (other_peer == peer) continue;
            if (other_peer == break_free) continue;  // just broke free this tick — skip
            const PlayerState& os = other_pl.GetState();
            if (os.kill_respawn_ticks > 0 || os.respawn_grace_ticks > 0 || os.finished) continue;
            if (os.grabbed) continue;       // already grabbed by someone else
            if (os.magneting) continue;     // magneting players can't be grabbed
            const float ox = os.x + TILE_SIZE * 0.5f;
            const float oy = os.y + TILE_SIZE * 0.5f;
            const float d2 = (mx - ox) * (mx - ox) + (my - oy) * (my - oy);
            if (d2 < best_dist2) {
                best_dist2 = d2;
                best = other_peer;
            }
        }
        if (best) {
            grab_targets_[peer] = best;
            auto tit = players_.find(best);
            if (tit != players_.end()) {
                PlayerState s = tit->second.GetState();
                s.grabbed = true;
                tit->second.SetState(s);
            }
        }
    }

    // 3. Snap each grabbed player on top of the grabber and clamp to world.
    //    Release immediately if the grabbed player ends up against a horizontal wall.
    const World& world = level_mgr_.GetWorld();
    std::vector<ENetPeer*> wall_release;
    for (auto& [grabber, grabbed] : grab_targets_) {
        auto git = players_.find(grabber);
        auto tit = players_.find(grabbed);
        if (git == players_.end() || tit == players_.end()) continue;

        const PlayerState& gs = git->second.GetState();
        PlayerState ts = tit->second.GetState();
        // Place the grabbed player on top of the grabber (one tile above).
        const float snap_x = gs.x;
        ts.x = snap_x;
        ts.y = gs.y - static_cast<float>(TILE_SIZE);
        ts.vel_x = 0.f;
        ts.vel_y = 0.f;
        ts.move_vel_x = 0.f;
        // Resolve any solid-tile overlap so the grabbed player doesn't clip into walls.
        ClampToWorld(ts, world);
        // If ClampToWorld had to shift the player horizontally, they hit a wall — release.
        // Any horizontal correction is at least 1 px; 0.5f safely detects any real adjustment.
        const float dx_wall = ts.x - snap_x;
        if (dx_wall < -0.5f || dx_wall > 0.5f) {
            wall_release.push_back(grabber);
            continue;
        }
        tit->second.SetState(ts);
    }
    for (ENetPeer* g : wall_release) ReleaseGrab(g);
}

void ServerSession::ResolvePlayerCollisions(const World& world) {
    // Collect mutable states.
    std::vector<std::pair<ENetPeer*, PlayerState>> states;
    states.reserve(players_.size());
    for (auto& [peer, pl] : players_)
        states.push_back({peer, pl.GetState()});

    for (size_t i = 0; i < states.size(); ++i) {
        for (size_t j = i + 1; j < states.size(); ++j) {
            PlayerState& a = states[i].second;
            PlayerState& b = states[j].second;

            // Skip dead / respawning players.
            if (a.kill_respawn_ticks > 0 || b.kill_respawn_ticks > 0) continue;

            // Skip grabbed players — their position is managed by ApplyMagnetGrab.
            if (a.grabbed || b.grabbed) continue;

            const float ax0 = a.x, ax1 = a.x + TILE_SIZE;
            const float ay0 = a.y, ay1 = a.y + TILE_SIZE;
            const float bx0 = b.x, bx1 = b.x + TILE_SIZE;
            const float by0 = b.y, by1 = b.y + TILE_SIZE;

            if (ax1 <= bx0 || bx1 <= ax0 || ay1 <= by0 || by1 <= ay0) continue;

            // Compute overlap on each axis.
            const float ov_x = (ax0 < bx0) ? (ax1 - bx0) : (bx1 - ax0);
            const float ov_y = (ay0 < by0) ? (ay1 - by0) : (by1 - ay0);

            // --- Dash push: dashing player slams the other with 2× dash force ---
            const bool a_dashing = a.dash_active_ticks > 0;
            const bool b_dashing = b.dash_active_ticks > 0;
            if (a_dashing || b_dashing) {
                const float push = DASH_SPEED * DASH_PUSH_MULTIPLIER;
                if (a_dashing && !b_dashing) {
                    b.vel_x      = a.dash_dir_x * push;
                    b.vel_y      = a.dash_dir_y * push;
                    b.move_vel_x = 0.f;
                } else if (b_dashing && !a_dashing) {
                    a.vel_x      = b.dash_dir_x * push;
                    a.vel_y      = b.dash_dir_y * push;
                    a.move_vel_x = 0.f;
                } else {
                    // Both dashing — mutual push, both dashes cancelled
                    const float a_dx = a.dash_dir_x, a_dy = a.dash_dir_y;
                    const float b_dx = b.dash_dir_x, b_dy = b.dash_dir_y;
                    a.vel_x      = b_dx * push;  a.vel_y      = b_dy * push;
                    a.move_vel_x = 0.f;
                    a.dash_active_ticks   = 0;
                    a.dash_cooldown_ticks = static_cast<uint8_t>(DASH_COOLDOWN_TICKS);
                    b.vel_x      = a_dx * push;  b.vel_y      = a_dy * push;
                    b.move_vel_x = 0.f;
                    b.dash_active_ticks   = 0;
                    b.dash_cooldown_ticks = static_cast<uint8_t>(DASH_COOLDOWN_TICKS);
                }
                // Position separation still applied below
            }

            if (ov_y < ov_x) {
                // Resolve vertically: one player stands on the other.
                // Also transfer horizontal velocity so the rider follows the carrier.
                if (ay0 < by0) {
                    a.y = by0 - static_cast<float>(TILE_SIZE);
                    if (a.vel_y > 0.f) a.vel_y = 0.f;
                    a.on_ground = true;
                    a.dash_cooldown_ticks = 0;
                    a.dash_ready = true;
                    a.x += (b.move_vel_x + b.vel_x) * FIXED_DT;
                } else {
                    b.y = ay0 - static_cast<float>(TILE_SIZE);
                    if (b.vel_y > 0.f) b.vel_y = 0.f;
                    b.on_ground = true;
                    b.dash_cooldown_ticks = 0;
                    b.dash_ready = true;
                    b.x += (a.move_vel_x + a.vel_x) * FIXED_DT;
                }
            } else {
                // Resolve horizontally: push both players apart equally.
                const float half = ov_x * 0.5f;
                if (ax0 < bx0) { a.x -= half; b.x += half; }
                else           { a.x += half; b.x -= half; }
            }
        }
    }

    // Clamp all players against solid tiles (prevents push into walls).
    for (auto& [peer, s] : states)
        ClampToWorld(s, world);

    // Write resolved states back.
    for (auto& [peer, s] : states) {
        auto it = players_.find(peer);
        if (it != players_.end()) it->second.SetState(s);
    }
}
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
    pkt.coop_wins    = static_cast<uint8_t>(coop_cleared_levels_);
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
