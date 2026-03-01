#pragma once
#include <cstdint>
#include "InputFrame.h"
#include "PlayerState.h"
#include "GameState.h"

// ---------------------------------------------------------------------------
// Versione del gioco (display) e versione del protocollo (compatibilità).
//
// GAME_VERSION  — stringa human-readable mostrata nel menu e nei log.
// PROTOCOL_VERSION — intero incrementato a ogni breaking change di protocollo
//                    (strutture pacchetti, PlayerState, logica di simulazione).
//                    Client e server devono avere lo stesso valore per giocare.
// ---------------------------------------------------------------------------
static constexpr const char*  GAME_VERSION     = "0.1.0";
static constexpr uint16_t     PROTOCOL_VERSION = 1;

// ---------------------------------------------------------------------------
// Costanti di protocollo condivise tra client e server.
// ---------------------------------------------------------------------------

static constexpr uint16_t SERVER_PORT       = 58291;  // server dedicato (online)
static constexpr uint16_t SERVER_PORT_LOCAL = 58721;  // server locale per la modalità offline
static constexpr size_t   MAX_CLIENTS      = static_cast<size_t>(MAX_PLAYERS);
static constexpr uint8_t  CHANNEL_RELIABLE = 0;
static constexpr uint8_t  CHANNEL_COUNT    = 1;

// Percorso della mappa di lobby: sempre il primo livello caricato dal server.
static constexpr const char* LOBBY_MAP_PATH = "assets/levels/_lobby.txt";

// Codici di disconnessione inviati come 'data' in enet_peer_disconnect().
// I 16 bit alti possono contenere informazioni extra (es. server_version).
// Layout: bits[7:0] = reason code,  bits[31:16] = payload extra
enum DisconnectReason : uint32_t {
    DISCONNECT_GENERIC          = 0,  // disconnessione normale / non specificata
    DISCONNECT_VERSION_MISMATCH = 1,  // incompatibilità di protocollo; bits[31:16] = server PROTOCOL_VERSION
    DISCONNECT_SERVER_BUSY      = 2,  // partita già in corso, riprovare più tardi
};

// Tipi di pacchetto (primo byte di ogni payload).
enum PktType : uint8_t {
    PKT_INPUT             = 1,   // Client --> Server: InputFrame da simulare
    PKT_PLAYER_STATE      = 2,   // (legacy, non usato dal passo 14 in poi)
    PKT_GAME_STATE        = 3,   // Server --> Client: GameState broadcast (tutti i player)
    PKT_WELCOME           = 4,   // Server --> Client: player_id assegnato al momento della connessione
    PKT_PLAYER_INFO       = 5,   // Client --> Server: nome + protocol_version (inviato dopo PKT_WELCOME)
    PKT_RESTART           = 6,   // Client --> Server: riparti dal via (reset pos + timer)
    PKT_LOAD_LEVEL        = 7,   // Server --> Client: carica il livello successivo (o fine partita)
    PKT_VERSION_MISMATCH  = 8,   // Server --> Client: versione incompatibile, aggiorna il gioco
    PKT_SERVER_BUSY       = 9,   // Server --> Client: sessione già in corso, ritenta più tardi
    PKT_LEVEL_RESULTS     = 10,  // Server --> Client: classifica al termine di ogni livello
    PKT_READY             = 11,  // Client --> Server: giocatore pronto per il livello successivo
};

// Client --> Server: inviato ogni tick fisso.
struct PktInput {
    uint8_t    type  = PKT_INPUT;
    InputFrame frame = {};
};

// Server --> Client: stato autoritativo dopo Player::Simulate (tutti i player).
struct PktGameState {
    uint8_t   type  = PKT_GAME_STATE;
    GameState state = {};
};

// Server --> Client: inviato una sola volta al momento della connessione.
struct PktWelcome {
    uint8_t  type          = PKT_WELCOME;
    uint32_t player_id     = 0;
    uint32_t session_token = 0;  // ID univoco della sessione; 0 = in lobby, != 0 = partita in corso
};

// Client --> Server: inviato subito dopo aver ricevuto PKT_WELCOME.
// Porta anche la protocol_version: il server confronta e disconnette se diversa.
struct PktPlayerInfo {
    uint8_t  type             = PKT_PLAYER_INFO;
    uint16_t protocol_version = PROTOCOL_VERSION;
    char     name[16]         = {};
};

// Client --> Server: richiesta di ripartire dal via (Backspace / Triangolo).
struct PktRestart {
    uint8_t type = PKT_RESTART;
};

// Server --> Client: carica il livello successivo o termina la partita.
struct PktLoadLevel {
    uint8_t type    = PKT_LOAD_LEVEL;
    uint8_t is_last = 0;      // 1 = nessun livello successivo, tornare al menu
    char    path[64] = {};    // percorso relativo del nuovo livello (es. "assets/levels/level_02.txt")
};

// Server --> Client: versione protocollo incompatibile.
// Il server invia questo pacchetto poi disconnette immediatamente.
struct PktVersionMismatch {
    uint8_t  type            = PKT_VERSION_MISMATCH;
    uint16_t server_version  = PROTOCOL_VERSION;  // versione del server (info per il client)
};

// Server --> Client: sessione già in corso, connessione rifiutata.
// Il server manda DISCONNECT con data=DISCONNECT_SERVER_BUSY (nessun pacchetto applicativo).
struct PktServerBusy {
    uint8_t type = PKT_SERVER_BUSY;
};

// Un'entry della classifica di fine livello.
struct ResultEntry {
    uint32_t player_id   = 0;
    uint32_t level_ticks = 0;
    char     name[16]    = {};
    uint8_t  finished    = 0;   // 1 = ha raggiunto l'exit, 0 = DNF
    uint8_t  _pad[3]     = {};
};

// Server --> Client: classifica inviata al termine di ogni livello (non lobby).
// Il client la mostra per RESULTS_DURATION_S secondi; premendo Ready si può saltare.
struct PktLevelResults {
    uint8_t     type    = PKT_LEVEL_RESULTS;
    uint8_t     count   = 0;          // numero di entry
    uint8_t     level   = 0;          // numero del livello appena completato
    uint8_t     _pad    = 0;
    ResultEntry entries[MAX_PLAYERS]; // ordinate: finished first, poi per tempo crescente
};

// Client --> Server: giocatore pronto per il livello successivo.
// Se tutti inviano PKT_READY prima della scadenza, si salta la schermata.
struct PktReady {
    uint8_t type = PKT_READY;
};
