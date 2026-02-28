#pragma once
#include <cstdint>
#include "InputFrame.h"
#include "PlayerState.h"
#include "GameState.h"

// ---------------------------------------------------------------------------
// Costanti di protocollo condivise tra client e server.
// ---------------------------------------------------------------------------

static constexpr uint16_t SERVER_PORT      = 7777;
static constexpr size_t   MAX_CLIENTS      = static_cast<size_t>(MAX_PLAYERS);
static constexpr uint8_t  CHANNEL_RELIABLE = 0;
static constexpr uint8_t  CHANNEL_COUNT    = 1;

// Tipi di pacchetto (primo byte di ogni payload).
enum PktType : uint8_t {
    PKT_INPUT        = 1,   // Client → Server: InputFrame da simulare
    PKT_PLAYER_STATE = 2,   // (legacy, non usato dal passo 14 in poi)
    PKT_GAME_STATE   = 3,   // Server → Client: GameState broadcast (tutti i player)
    PKT_WELCOME      = 4,   // Server → Client: player_id assegnato al momento della connessione
    PKT_PLAYER_INFO  = 5,   // Client → Server: nome del player (inviato dopo PKT_WELCOME)
};

// Client → Server: inviato ogni tick fisso.
struct PktInput {
    uint8_t    type  = PKT_INPUT;
    InputFrame frame = {};
};

// Server → Client: stato autoritativo dopo Player::Simulate (tutti i player).
struct PktGameState {
    uint8_t   type  = PKT_GAME_STATE;
    GameState state = {};
};

// Server → Client: inviato una sola volta al momento della connessione.
struct PktWelcome {
    uint8_t  type      = PKT_WELCOME;
    uint32_t player_id = 0;
};

// Client → Server: inviato subito dopo aver ricevuto PKT_WELCOME.
struct PktPlayerInfo {
    uint8_t type    = PKT_PLAYER_INFO;
    char    name[16] = {};
};
