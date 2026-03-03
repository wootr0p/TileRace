#pragma once
#include "PlayerState.h"
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
};
