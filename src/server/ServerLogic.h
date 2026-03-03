#pragma once
#include <cstdint>
#include <atomic>

// Blocking ENet server loop. Caller must call enet_initialize() beforehand.
// Returns only when stop_flag is set to true.
void RunServer(uint16_t port, const char* map_path, std::atomic<bool>& stop_flag);
