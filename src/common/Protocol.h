#pragma once
#include <cstdint>
#include "InputFrame.h"
#include "GameState.h"

// =============================================================================
// Protocol.h — Contratto di comunicazione client ↔ server
// =============================================================================
//
// Questo file è il "dizionario" del protocollo di rete.
// REGOLA FONDAMENTALE: ogni struct qui dentro deve essere POD (Plain Old Data),
// senza puntatori, senza virtual, senza std::string.
// Perché? Perché viene serializzato direttamente in byte e mandato su rete.
//
// ATTENZIONE ARCHITETTURALE: in produzione si userebbe una serializzazione
// esplicita (es. Flatbuffers) per gestire endianness e padding.
// Per ora usiamo struct packed per semplicità didattica.
// =============================================================================

#pragma pack(push, 1)  // Disabilita il padding del compilatore per questi struct

// -------------------------
// Tipi di pacchetto
// -------------------------
enum PacketType : uint8_t {
    // Legacy (da rimuovere gradualmente)
    PACKET_PING = 1,
    PACKET_PONG = 2,

    // Handshake
    PACKET_CONNECT_REQUEST  = 3,  // Client → Server: "voglio entrare"
    PACKET_PLAYER_ASSIGNED  = 4,  // Server → Client: "sei il giocatore N"

    // Ciclo di gioco principale
    PACKET_INPUT            = 5,  // Client → Server: InputFrame (ogni tick)
    PACKET_GAME_STATE       = 6,  // Server → Client: GameState autoritativo

    // Gestione sessione
    PACKET_PLAYER_JOINED    = 7,  // Server → tutti: nuovo giocatore
    PACKET_PLAYER_LEFT      = 8,  // Server → tutti: giocatore disconnesso

    // Audio event-based: il server decide QUANDO, il client decide COME
    PACKET_AUDIO_EVENT      = 9,
};

// -------------------------
// Audio Events
// -------------------------
enum AudioEventId : uint8_t {
    AUDIO_JUMP    = 1,
    AUDIO_LAND    = 2,
    AUDIO_COLLECT = 3,
    AUDIO_DEATH   = 4,
};

// -------------------------
// Strutture dei pacchetti
// -------------------------

struct PacketHeader {
    uint8_t type;  // PacketType
};

struct PktConnectRequest {
    PacketHeader header;
    uint8_t  protocol_version = 1;
    uint32_t session_token    = 0;  // 0 = server pubblico; altrimenti deve combaciare
};

struct PktPlayerAssigned {
    PacketHeader header;
    uint32_t     player_id;
};

struct PktInput {
    PacketHeader header;
    InputFrame   frame;
};

struct PktGameState {
    PacketHeader header;
    GameState    state;
};

struct PktPlayerJoined {
    PacketHeader header;
    uint32_t     player_id;
};

struct PktPlayerLeft {
    PacketHeader header;
    uint32_t     player_id;
};

struct PktAudioEvent {
    PacketHeader header;
    uint32_t     player_id;  // Chi ha scatenato l'evento (per audio posizionale)
    AudioEventId event_id;
};

// Legacy — solo per compatibilità durante la migrazione
struct NetworkMessage {
    uint8_t type;
    int32_t value;
};

#pragma pack(pop)