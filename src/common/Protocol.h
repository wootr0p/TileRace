#pragma once
#include <cstdint>
#include "InputFrame.h"
#include "PlayerState.h"
#include "GameState.h"

// Increment PROTOCOL_VERSION on any breaking change to packet layout, PlayerState,
// or simulation behaviour so client and server can detect incompatibility at connect time.
static constexpr const char*  GAME_VERSION     = "0.2.0b";
static constexpr uint16_t     PROTOCOL_VERSION = 5;

static constexpr uint16_t SERVER_PORT       = 58291;  // dedicated (online) server
static constexpr uint16_t SERVER_PORT_LOCAL = 58721;  // in-process server for offline mode
static constexpr size_t   MAX_CLIENTS      = static_cast<size_t>(MAX_PLAYERS);
static constexpr uint8_t  CHANNEL_RELIABLE = 0;
static constexpr uint8_t  CHANNEL_COUNT    = 1;

// First map loaded on server start; players wait here between games.
static constexpr const char* LOBBY_MAP_PATH = "assets/levels/tilemaps/_Lobby.tmj";

// ENet disconnect reason codes (32-bit). Bits[7:0] = reason; bits[31:16] = optional payload.
enum DisconnectReason : uint32_t {
    DISCONNECT_GENERIC          = 0,
    DISCONNECT_VERSION_MISMATCH = 1,  // bits[31:16] = server PROTOCOL_VERSION
    DISCONNECT_SERVER_BUSY      = 2,
};

// First byte of every packet identifies its type.
enum PktType : uint8_t {
    PKT_INPUT             = 1,   // C → S  one InputFrame per fixed tick
    PKT_PLAYER_STATE      = 2,   // (legacy, unused)
    PKT_GAME_STATE        = 3,   // S → C  authoritative GameState broadcast
    PKT_WELCOME           = 4,   // S → C  assigned player_id + session_token
    PKT_PLAYER_INFO       = 5,   // C → S  name + protocol_version (sent right after PKT_WELCOME)
    PKT_RESTART           = 6,   // C → S  respawn at last checkpoint (or spawn if none); Backspace / Circle
    PKT_LOAD_LEVEL        = 7,   // S → C  load next level or return to menu (is_last=1)
    PKT_VERSION_MISMATCH  = 8,   // S → C  incompatible version; server disconnects immediately after
    PKT_SERVER_BUSY       = 9,   // S → C  session in progress, retry later
    PKT_LEVEL_RESULTS     = 10,  // S → C  end-of-level leaderboard
    PKT_READY             = 11,  // C → S  player ready for the next level
    PKT_GLOBAL_RESULTS    = 12,  // S → C  session-end global leaderboard (wins per player)
    PKT_RESTART_SPAWN     = 13,  // C → S  respawn always at level spawn, ignoring checkpoints; Delete / Square
    PKT_LEVEL_DATA        = 14,  // S → C  generated level tile grid (variable-size packet)
    PKT_EMOTE             = 15,  // C → S  emote selection (emote_id 0-7)
    PKT_EMOTE_BROADCAST   = 16,  // S → C  broadcast emote to all clients
    PKT_GENERATING        = 17,  // S → C  server is generating the next level; client shows loading overlay
};

struct PktInput {
    uint8_t    type  = PKT_INPUT;
    InputFrame frame = {};
};

struct PktGameState {
    uint8_t   type  = PKT_GAME_STATE;
    GameState state = {};
};

// Sent exactly once on connection. session_token == 0 means currently in lobby.
struct PktWelcome {
    uint8_t  type          = PKT_WELCOME;
    uint32_t player_id     = 0;
    uint32_t session_token = 0;
};

// Sent immediately after receiving PKT_WELCOME. Server disconnects if protocol_version mismatches.
struct PktPlayerInfo {
    uint8_t  type             = PKT_PLAYER_INFO;
    uint16_t protocol_version = PROTOCOL_VERSION;
    char     name[16]         = {};
};

struct PktRestart {
    uint8_t type = PKT_RESTART;
};

// Always resets to the level spawn point, discarding any saved checkpoint.
struct PktRestartSpawn {
    uint8_t type = PKT_RESTART_SPAWN;
};

struct PktLoadLevel {
    uint8_t type     = PKT_LOAD_LEVEL;
    uint8_t is_last  = 0;       // 1 = no more levels; client returns to main menu
    char    path[64] = {};      // relative path, e.g. "assets/levels/tilemaps/Level02.tmj"
};

struct PktVersionMismatch {
    uint8_t  type           = PKT_VERSION_MISMATCH;
    uint16_t server_version = PROTOCOL_VERSION;
};

struct PktServerBusy {
    uint8_t type = PKT_SERVER_BUSY;
};

// One leaderboard entry. Sorted: finished players first, then by level_ticks ascending.
struct ResultEntry {
    uint32_t player_id   = 0;
    uint32_t level_ticks = 0;
    char     name[16]    = {};
    uint8_t  finished    = 0;   // 1 = reached exit tile; 0 = DNF
    uint8_t  _pad[3]     = {};
};

// Sent at end of every non-lobby level. Displayed for RESULTS_DURATION_S; skippable with PKT_READY.
struct PktLevelResults {
    uint8_t     type    = PKT_LEVEL_RESULTS;
    uint8_t     count   = 0;
    uint8_t     level   = 0;
    uint8_t     _pad    = 0;
    ResultEntry entries[MAX_PLAYERS];
};

struct PktReady {
    uint8_t type = PKT_READY;
};

// One global leaderboard entry — counts of 1st-place finishes across all levels.
struct GlobalResultEntry {
    uint32_t player_id = 0;
    uint32_t wins      = 0;   // number of levels where this player finished 1st
    char     name[16]  = {};
};

// Sent by the server after the last level, before PKT_LOAD_LEVEL(is_last=1).
// Clients display a session-summary screen then send PKT_READY to proceed.
struct PktGlobalResults {
    uint8_t           type         = PKT_GLOBAL_RESULTS;
    uint8_t           count        = 0;    // number of valid entries
    uint8_t           total_levels = 0;   // how many levels were played this session
    uint8_t           _pad         = 0;
    GlobalResultEntry entries[MAX_PLAYERS];
};

// Variable-size packet: header followed by width*height bytes of tile chars.
// Sent by the server when a generated level is loaded (chunk-based level generator).
// The client reconstructs the World from the char grid (solid = (ch == '0')).
struct PktLevelDataHeader {
    uint8_t  type    = PKT_LEVEL_DATA;
    uint8_t  is_last = 0;       // 1 → session over, return to menu
    uint16_t width   = 0;       // map width in tiles
    uint16_t height  = 0;       // map height in tiles
    uint8_t  level   = 0;       // current level number (for display)
    uint8_t  _pad    = 0;
    // char data[width * height] follows immediately
};

// Number of generated levels per session before returning to lobby.
static constexpr int MAX_GENERATED_LEVELS = 8;

// Emote system — 8 directional emotes (mapped clockwise from Up).
static constexpr int   EMOTE_COUNT      = 8;
static constexpr float EMOTE_DURATION    = 2.0f;   // seconds the bubble is visible
static constexpr const char* EMOTE_TEXTS[EMOTE_COUNT] = {
    "?", "!", "u.u", ":D", "xD", ";)", "T.T", "=)"
};
// Direction labels for the wheel (Up, UpRight, Right, DownRight, Down, DownLeft, Left, UpLeft)

struct PktEmote {
    uint8_t type     = PKT_EMOTE;
    uint8_t emote_id = 0;  // 0-7
};

struct PktEmoteBroadcast {
    uint8_t  type      = PKT_EMOTE_BROADCAST;
    uint8_t  emote_id  = 0;  // 0-7
    uint32_t player_id = 0;
};

// Sent before PKT_LEVEL_DATA to warn clients that generation may take a moment.
// Clients show a loading overlay and extend their disconnect timeout.
struct PktGenerating {
    uint8_t type  = PKT_GENERATING;
    uint8_t level = 0;   // the level number being generated
};
