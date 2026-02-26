#pragma once
#include <cstdint>

// Identificatori per i tipi di pacchetto
enum PacketType : uint8_t {
    PACKET_PING = 1,
    PACKET_PONG = 2
};

// Una struttura fissa che entrambi i moduli possono leggere
struct NetworkMessage {
    uint8_t type;
    int32_t value;
};