#pragma once
#include <cstdint>
#include <atomic>

// Avvia il loop ENet del server (bloccante finché stop_flag diventa true).
// Presuppone che enet_initialize() sia già stato chiamato dal chiamante.
void RunServer(uint16_t port, const char* map_path, std::atomic<bool>& stop_flag);
