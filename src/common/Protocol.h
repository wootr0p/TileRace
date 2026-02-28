#pragma once
#include <cstdint>
#include "InputFrame.h"
#include "PlayerState.h"
#include "GameState.h"

// ---------------------------------------------------------------------------
// Costanti di protocollo condivise tra client e server.
// ---------------------------------------------------------------------------

static constexpr uint16_t SERVER_PORT      = 47777;
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
    PKT_RESTART      = 6,   // Client → Server: riparti dal via (reset pos + timer)
    PKT_LOAD_LEVEL   = 7,   // Server → Client: carica il livello successivo (o fine partita)
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

// Client → Server: richiesta di ripartire dal via (Backspace / Triangolo).
struct PktRestart {
    uint8_t type = PKT_RESTART;
};

// Server → Client: carica il livello successivo o termina la partita.
struct PktLoadLevel {
    uint8_t type    = PKT_LOAD_LEVEL;
    uint8_t is_last = 0;      // 1 = nessun livello successivo, tornare al menu
    char    path[64] = {};    // percorso relativo del nuovo livello (es. "assets/levels/level_02.txt")
};
