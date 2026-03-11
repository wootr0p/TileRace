#pragma once
#include <cstdint>

// Game session mode — determines collision rules, checkpoint behavior, HUD, and results screens.
// Stored in GameState and broadcast every tick so all clients stay in sync.
enum class GameMode : uint8_t {
    COOP    = 0,   // shared checkpoints, player collisions, cooperative clear
    RACE    = 1,   // no collisions, no checkpoints, individual race
    VERSUS  = 2,   // player collisions + magnet grab, no checkpoints, timer preserved on respawn; selected by default in lobby
};
