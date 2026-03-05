#pragma once
// SRP: game session state machine for the server.
// Manages connected players, lobby / game / results phases, and level progression.
// Socket lifecycle and the ENet service loop live in RunServer (ServerLogic.cpp).
#include "LevelManager.h"
#include "ChunkStore.h"
#include "Player.h"
#include "Protocol.h"
#include "GameMode.h"
#include <enet/enet.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstdint>

class ServerSession {
public:
    // Load the initial map. Check IsReady() after construction.
    // When skip_lobby is true the lobby is skipped: level 1 is generated immediately.
    // initial_mode sets the starting game mode (used by offline → RACE).
    ServerSession(const char* initial_map_path, int initial_level,
                  bool skip_lobby = false, GameMode initial_mode = GameMode::COOP);

    bool IsReady() const { return is_ready_; }

    // ENet event handlers — called by RunServer inside the service loop.

    void OnConnect(ENetHost* host, ENetPeer* peer);

    // Returns true when a level change was triggered (caller must break the inner event loop).
    bool OnDisconnect(ENetHost* host, ENetPeer* peer);

    // Returns true when a level change was triggered.
    bool OnReceive(ENetHost* host, ENetPeer* peer, const uint8_t* data, size_t len);

    // Periodic timer checks (results timeout). Call once per outer loop iteration.
    void CheckTimers(ENetHost* host);

private:
    bool HandleInput     (ENetHost* host, ENetPeer* peer, const PktInput& pkt);
    void HandlePlayerInfo(ENetHost* host, ENetPeer* peer, const PktPlayerInfo& pkt);
    void HandleRestart   (ENetPeer* peer);       // respawn at last checkpoint (or spawn)
    void HandleRestartSpawn(ENetPeer* peer);     // respawn always at level spawn
    bool HandleReady     (ENetHost* host, ENetPeer* peer);  // returns true on level change
    void HandleSetGameMode(ENetPeer* peer, const PktSetGameMode& pkt);
    bool HandleStartGame (ENetHost* host, ENetPeer* peer);  // returns true on level change

    // Load next map, reset all players, broadcast new state.
    void DoLevelChange(ENetHost* host);
    // Broadcast PKT_GLOBAL_RESULTS and enter the global-results phase.
    void SendGlobalResults(ENetHost* host);
    // Send PKT_LOAD_LEVEL(is_last=1) then tear down the session.
    void FinishSession(ENetHost* host);
    // Disconnect all peers, reload the lobby (called when is_last).
    void ResetToInitial(ENetHost* host);
    void SendResults   (ENetHost* host, const char* reason);
    void BroadcastGameState(ENetHost* host);
    void BroadcastLevelData(ENetHost* host);      // send PKT_LEVEL_DATA with generated world grid
    void BroadcastGenerating(ENetHost* host);     // send PKT_GENERATING before level generation starts
    void SendLevelDataToPeer(ENetPeer* peer);     // send PKT_LEVEL_DATA to a single peer
    void UpdateZone();
    bool AllInZone()        const;
    uint32_t CountdownTicks() const;
    PlayerState ApplySpawnReset(PlayerState s, bool with_kill) const;
    void ResolvePlayerCollisions(const World& world);   // coop mode: push overlapping player AABBs apart
    void ApplyMagnetGrab(ENetPeer* break_free = nullptr);  // magnet holders grab & carry nearby players
    void ReleaseGrab(ENetPeer* grabber);                 // release a grabbed player (if any)

    // Leader election: elect a new leader from the remaining players.
    void ElectLeader();

    LevelManager level_mgr_;
    ChunkStore   chunk_store_;       // loaded at construction; used by LevelGenerator

    std::string  initial_map_path_;
    int          initial_level_;
    int          current_level_;
    bool         skip_lobby_          = false;
    uint32_t     coop_cleared_levels_ = 0;   // number of levels fully cleared by the team

    bool         is_ready_           = false;
    bool         in_lobby_            = false;
    bool         game_locked_         = false;
    bool         in_results_          = false;
    bool         in_global_results_   = false;  // true while showing session-end global leaderboard

    GameMode     game_mode_           = GameMode::COOP;
    uint32_t     leader_id_           = 0;      // player_id of the current session leader

    uint32_t     session_token_           = 0u;
    uint32_t     next_player_id_          = 1u;
    uint32_t     results_start_ms_        = 0u;
    uint32_t     global_results_start_ms_ = 0u;
    uint32_t     zone_start_ms_           = 0u;
    uint32_t     level_start_ms_          = 0u;

    std::unordered_map<ENetPeer*, Player>   players_;
    std::unordered_map<ENetPeer*, uint32_t> best_ticks_;
    std::unordered_set<ENetPeer*>           ready_peers_;
    // Persistent within a session (survive per-level resets); cleared by ResetToInitial.
    std::unordered_map<uint32_t, uint32_t>  session_wins_;   // player_id → 1st-place count
    std::unordered_map<uint32_t, std::string> session_names_; // player_id → display name

    // Shared checkpoint tracking — each activated spawn-position is recorded so that
    // re-visiting an already-activated checkpoint doesn't reset everyone's progress.
    std::vector<std::pair<float,float>> activated_checkpoints_;

    // Magnet-grab relationships: grabber peer → grabbed peer.
    std::unordered_map<ENetPeer*, ENetPeer*> grab_targets_;

    static constexpr uint32_t LEVEL_TIME_LIMIT_MS        = 120'000u;
    static constexpr uint32_t NEXT_LEVEL_MS              =   3'000u;
    static constexpr uint32_t RESULTS_DURATION_MS        =  15'000u;
    static constexpr uint32_t GLOBAL_RESULTS_DURATION_MS =  25'000u;  // longer: final screen
};
