#pragma once
#include "PlayerState.h"
#include "GameMode.h"
#include <cstdint>

// Max simultaneous players; mirrors MAX_CLIENTS in the ENet host setup.
static constexpr int MAX_PLAYERS = 8;

// Full-world authoritative snapshot broadcast by the server every tick.
// Each contained PlayerState also carries last_processed_tick for client-side reconciliation.
struct GameState {
    uint32_t    count                      = 0;   // number of connected players
    PlayerState players[MAX_PLAYERS]       = {};
    uint32_t    next_level_countdown_ticks = 0;   // > 0: ticks until automatic level change
    uint32_t    time_limit_secs            = 0;   // remaining seconds of the 2-minute time limit
    uint8_t     is_lobby                   = 0;   // 1 when the active map is _lobby.txt
    uint8_t     game_mode                  = static_cast<uint8_t>(GameMode::COOP);
    uint8_t     max_generated_levels       = 5;   // authoritative session setting (leader can change in lobby)
    uint8_t     pad[1]                     = {};
    uint32_t    leader_id                  = 0;   // player_id of the current session leader
};
