#pragma once
#include <cstdint>
#include <atomic>
#include "GameMode.h"

// Blocking ENet server loop. Caller must call enet_initialize() beforehand.
// Returns only when stop_flag is set to true.
// When skip_lobby is true the server generates level 1 immediately (no lobby).
// initial_mode sets the starting game mode (RACE for offline, VERSUS for online).
void RunServer(uint16_t port, const char* map_path, std::atomic<bool>& stop_flag,
               bool skip_lobby = false, GameMode initial_mode = GameMode::VERSUS);
