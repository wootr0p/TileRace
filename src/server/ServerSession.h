#pragma once
// SRP: game session state machine for the server.
// Manages connected players, lobby / game / results phases, and level progression.
// Socket lifecycle and the ENet service loop live in RunServer (ServerLogic.cpp).
#include "LevelManager.h"
#include "Player.h"
#include "Protocol.h"
#include <enet/enet.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstdint>

class ServerSession {
public:
    // Load the initial map. Check IsReady() after construction.
    ServerSession(const char* initial_map_path, int initial_level);

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
    void HandleRestart   (ENetPeer* peer);
    bool HandleReady     (ENetHost* host, ENetPeer* peer);  // returns true on level change

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
    void UpdateZone();
    bool AllInZone()        const;
    uint32_t CountdownTicks() const;
    PlayerState ApplySpawnReset(PlayerState s, bool with_kill) const;

    LevelManager level_mgr_;

    std::string  initial_map_path_;
    int          initial_level_;
    int          current_level_;

    bool         is_ready_           = false;
    bool         in_lobby_            = false;
    bool         game_locked_         = false;
    bool         in_results_          = false;
    bool         in_global_results_   = false;  // true while showing session-end global leaderboard

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

    static constexpr uint32_t LEVEL_TIME_LIMIT_MS        = 120'000u;
    static constexpr uint32_t NEXT_LEVEL_MS              =   3'000u;
    static constexpr uint32_t RESULTS_DURATION_MS        =  15'000u;
    static constexpr uint32_t GLOBAL_RESULTS_DURATION_MS =  25'000u;  // longer: final screen
};
