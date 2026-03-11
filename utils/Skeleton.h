/*
 * ============================================================================
 * SKELETON.H  —  AI Context Snapshot for TileRace
 * Generated : 2026-03-11 00:09
 * ============================================================================
 *
 * PURPOSE
 *   This file is a machine-generated, read-only summary of every public header
 *   in the TileRace project.  It is designed to give an AI assistant immediate,
 *   full-project context without requiring access to the individual source files.
 *
 * HOW TO USE THIS FILE
 *   This file covers the complete public interface of the codebase.
 *   Inline function bodies have been stripped; class / struct / enum bodies are kept.
 *   If you need deeper context — game mechanics, flow, physics constants, network
 *   protocol semantics, implementation status — read PROJECT_CONTEXT.md located in
 *   the utils/ folder (same directory as this file).  That document is the
 *   authoritative human+AI reference for the whole project.
 *
 * IMPORTANT — AI INSTRUCTIONS
 *   After EVERY code change (new feature, refactor, bug fix, constant tweak,
 *   protocol change, or any modification to architecture / mechanics / status),
 *   you MUST do BOTH of the following — always, without exception:
 *     1. Update utils/PROJECT_CONTEXT.md to reflect the current state of the project.
 *     2. Run utils/generate_skeleton.py to regenerate this file:
 *           python utils/generate_skeleton.py
 *   Keep PROJECT_CONTEXT.md and Skeleton.h accurate and in sync with the codebase
 *   at all times.
 *
 * CONTENTS
 *   One section per .h file, in dependency order (common → server → client).
 *   Inline function / method BODIES have been stripped (replaced with
 *   "body stripped" markers) so only the interface is shown.
 *   All comments present in the original headers are preserved.
 *
 * PROJECT OVERVIEW
 *   TileRace — real-time multiplayer 2-D platformer
 *   Language  : C++20
 *   Graphics  : Raylib
 *   Networking: ENet (UDP, authoritative server)
 *   Build     : CMake + Ninja  (presets: run-scc-debug / run-scc-release)
 *   Layout:
 *     src/common/  — pure logic (no Raylib, no ENet): World, Player, Physics, Protocol
 *     src/server/  — authoritative server: ServerSession, LevelManager, ServerLogic
 *     src/client/  — rendering + input + game loop: Renderer, GameSession, MainMenu, …
 *
 * ARCHITECTURE NOTES
 *   • Client-side prediction with server reconciliation (InputFrame + tick seq.)
 *   • Fixed timestep 60 Hz (FIXED_DT = 1/60 s) on both client and server
 *   • Lobby map (_lobby.txt) → levels level_01..N → results screen loop
 *   • ENet wrapped behind NetworkClient (client) / ServerSession (server)
 *   • SOLID refactoring applied: SRP, DIP, DRY throughout
 *
 * src/ DIRECTORY TREE
 *   ├── client
 *   │   ├── CMakeLists.txt
 *   │   ├── Colors.h
 *   │   ├── GameSession.cpp
 *   │   ├── GameSession.h
 *   │   ├── HudCoop.cpp
 *   │   ├── HudCoop.h
 *   │   ├── HudRace.cpp
 *   │   ├── HudRace.h
 *   │   ├── HudVersus.cpp
 *   │   ├── HudVersus.h
 *   │   ├── InputSampler.cpp
 *   │   ├── InputSampler.h
 *   │   ├── LevelPalette.h
 *   │   ├── LevelResultsCoop.cpp
 *   │   ├── LevelResultsCoop.h
 *   │   ├── LevelResultsRace.cpp
 *   │   ├── LevelResultsRace.h
 *   │   ├── LocalServer.cpp
 *   │   ├── LocalServer.h
 *   │   ├── main.cpp
 *   │   ├── MainMenu.cpp
 *   │   ├── MainMenu.h
 *   │   ├── NetworkClient.cpp
 *   │   ├── NetworkClient.h
 *   │   ├── Renderer.cpp
 *   │   ├── Renderer.h
 *   │   ├── SaveData.cpp
 *   │   ├── SaveData.h
 *   │   ├── SessionResultsCoop.cpp
 *   │   ├── SessionResultsCoop.h
 *   │   ├── SessionResultsRace.cpp
 *   │   ├── SessionResultsRace.h
 *   │   ├── SfxManager.cpp
 *   │   ├── SfxManager.h
 *   │   ├── SoundPool.cpp
 *   │   ├── SoundPool.h
 *   │   ├── UIWidgets.cpp
 *   │   ├── UIWidgets.h
 *   │   ├── VisualEffects.cpp
 *   │   ├── VisualEffects.h
 *   │   ├── WinIcon.cpp
 *   │   └── WinIcon.h
 *   ├── common
 *   │   ├── CMakeLists.txt
 *   │   ├── GameMode.h
 *   │   ├── GameState.h
 *   │   ├── InputFrame.h
 *   │   ├── Physics.h
 *   │   ├── Player.cpp
 *   │   ├── Player.h
 *   │   ├── PlayerState.h
 *   │   ├── Protocol.h
 *   │   ├── SpawnFinder.h
 *   │   ├── World.cpp
 *   │   └── World.h
 *   ├── server
 *   │   ├── ChunkStore.cpp
 *   │   ├── ChunkStore.h
 *   │   ├── CMakeLists.txt
 *   │   ├── LevelGenerator.cpp
 *   │   ├── LevelGenerator.h
 *   │   ├── LevelManager.cpp
 *   │   ├── LevelManager.h
 *   │   ├── LevelValidator.cpp
 *   │   ├── LevelValidator.h
 *   │   ├── main.cpp
 *   │   ├── PlayerReset.h
 *   │   ├── ServerLogic.cpp
 *   │   ├── ServerLogic.h
 *   │   ├── ServerSession.cpp
 *   │   └── ServerSession.h
 *   └── app_icon.rc.in
 *
 * HEADERS INCLUDED BELOW  (37 files)
 *   [01]  src/common/GameMode.h
 *   [02]  src/common/GameState.h
 *   [03]  src/common/InputFrame.h
 *   [04]  src/common/Physics.h
 *   [05]  src/common/Player.h
 *   [06]  src/common/PlayerState.h
 *   [07]  src/common/Protocol.h
 *   [08]  src/common/SpawnFinder.h
 *   [09]  src/common/World.h
 *   [10]  src/server/ChunkStore.h
 *   [11]  src/server/LevelGenerator.h
 *   [12]  src/server/LevelManager.h
 *   [13]  src/server/LevelValidator.h
 *   [14]  src/server/PlayerReset.h
 *   [15]  src/server/ServerLogic.h
 *   [16]  src/server/ServerSession.h
 *   [17]  src/client/Colors.h
 *   [18]  src/client/GameSession.h
 *   [19]  src/client/HudCoop.h
 *   [20]  src/client/HudRace.h
 *   [21]  src/client/HudVersus.h
 *   [22]  src/client/InputSampler.h
 *   [23]  src/client/LevelPalette.h
 *   [24]  src/client/LevelResultsCoop.h
 *   [25]  src/client/LevelResultsRace.h
 *   [26]  src/client/LocalServer.h
 *   [27]  src/client/MainMenu.h
 *   [28]  src/client/NetworkClient.h
 *   [29]  src/client/Renderer.h
 *   [30]  src/client/SaveData.h
 *   [31]  src/client/SessionResultsCoop.h
 *   [32]  src/client/SessionResultsRace.h
 *   [33]  src/client/SfxManager.h
 *   [34]  src/client/SoundPool.h
 *   [35]  src/client/UIWidgets.h
 *   [36]  src/client/VisualEffects.h
 *   [37]  src/client/WinIcon.h
 * ============================================================================
 */


// ==========================================================================
// FILE : GameMode.h
// PATH : src/common/GameMode.h
// ==========================================================================

#pragma once
#include <cstdint>

// Game session mode — determines collision rules, checkpoint behavior, HUD, and results screens.
// Stored in GameState and broadcast every tick so all clients stay in sync.
enum class GameMode : uint8_t {
    COOP    = 0,   // shared checkpoints, player collisions, cooperative clear
    RACE    = 1,   // no collisions, no checkpoints, individual race
    VERSUS  = 2,   // player collisions + magnet grab, no checkpoints, timer preserved on respawn; selected by default in lobby
};


// ==========================================================================
// FILE : GameState.h
// PATH : src/common/GameState.h
// ==========================================================================

#pragma once
#include "PlayerState.h"
#include "GameMode.h"
#include <cstdint>

// Max simultaneous players; mirrors MAX_CLIENTS in the ENet host setup.
static constexpr int MAX_PLAYERS = 8;

// Full-world authoritative snapshot broadcast by the server every tick.
// Each contained PlayerState also carries last_processed_tick for client-side reconciliation.
struct GameState {
    uint32_t    count                      = 0;   // number of connected players
    PlayerState players[MAX_PLAYERS]       = {};
    uint32_t    next_level_countdown_ticks = 0;   // > 0: ticks until automatic level change
    uint32_t    time_limit_secs            = 0;   // remaining seconds of the 2-minute time limit
    uint8_t     is_lobby                   = 0;   // 1 when the active map is _lobby.txt
    uint8_t     game_mode                  = static_cast<uint8_t>(GameMode::COOP);
    uint8_t     max_generated_levels       = 5;   // authoritative session setting (leader can change in lobby)
    uint8_t     pad[1]                     = {};
    uint32_t    leader_id                  = 0;   // player_id of the current session leader
};


// ==========================================================================
// FILE : InputFrame.h
// PATH : src/common/InputFrame.h
// ==========================================================================

#pragma once
#include <cstdint>

// Button bitmask. Rising-edge flags (JUMP_PRESS, DASH) are set in InputSampler::Poll()
// and cleared after one fixed tick so they fire exactly once per physical press.
enum InputBits : uint16_t {
    BTN_LEFT       = 1 << 0,  // move left (held)
    BTN_RIGHT      = 1 << 1,  // move right (held)
    BTN_JUMP       = 1 << 2,  // jump held; used for variable-height cut
    BTN_JUMP_PRESS = 1 << 3,  // rising edge — sets jump buffer
    BTN_DASH       = 1 << 4,  // rising edge — starts dash
    BTN_UP         = 1 << 5,  // held — dash direction / upward steer
    BTN_DOWN       = 1 << 6,  // held — dash direction / downward steer
    BTN_DRAW       = 1 << 7,  // held — player is drawing a trail on the map
    BTN_SPRINT     = 1 << 8,  // held — sprint (faster horizontal movement)
    BTN_MAGNET     = 1 << 9,  // held — magnet (attracts nearby players)
};

// Deterministic input snapshot for one 60 Hz fixed tick.
// Identical layout on client and server: same inputs + same PlayerState = same result.
struct InputFrame {
    uint32_t tick    = 0;     // monotonic fixed-tick counter (60 Hz)
    uint16_t buttons = 0;     // OR of InputBits

    // Raw direction for dash targeting and in-flight steering.
    // Normalised internally in Player::Simulate / RequestDash / SteerDash.
    float dash_dx = 0.f;
    float dash_dy = 0.f;

    // Horizontal movement axis [-1, 1]. Keyboard/DPAD = ±1.0; analogue stick = raw after deadzone.
    float move_x  = 0.f;

    bool Has(uint16_t b) const { return (buttons & b) != 0; }
};


// ==========================================================================
// FILE : Physics.h
// PATH : src/common/Physics.h
// ==========================================================================

#pragma once
// Compile-time simulation constants. Edit here to tune gameplay.
// No external dependencies — primitive types only.

// Geometry
inline constexpr int   TILE_SIZE   = 32;       // pixels per tile side

// Horizontal movement
inline constexpr float MOVE_SPEED  = 300.f;    // px/s  — max horizontal speed
inline constexpr float MOVE_ACCEL  = 8000.f;   // px/s² — acceleration toward target speed
inline constexpr float MOVE_DECEL  = 6000.f;   // px/s² — deceleration on stop or direction reversal

// Gravity / jump
inline constexpr float GRAVITY        = 2000.f; // px/s²
inline constexpr float JUMP_FORCE     = 1000.f; // upward velocity impulse on jump
inline constexpr float MAX_FALL_SPEED = 1400.f; // terminal velocity (downward)

// Fixed timestep
inline constexpr float FIXED_DT = 1.f / 60.f;  // seconds per tick

// Advanced jump mechanics
inline constexpr int   JUMP_BUFFER_TICKS   = 10;    // ticks: buffered jump input before landing (~167 ms)
inline constexpr int   COYOTE_TICKS        =  6;    // ticks: grace window after leaving a ledge (~100 ms)
inline constexpr float JUMP_CUT_MULTIPLIER = 0.45f; // vel_y multiplier when jump key released early
inline constexpr float WALL_JUMP_FORCE_X   = 507.f; // horizontal push from wall jump (px/s)
inline constexpr float WALL_JUMP_FRICTION  =   5.f; // exponential vel_x decay coefficient (1/s)
inline constexpr int   WALL_PROBE_REACH    = TILE_SIZE / 4; // px beyond player edge to detect adjacent walls (8 px)

// Dash
inline constexpr float DASH_SPEED          = 1200.f; // px/s during active dash
inline constexpr int   DASH_ACTIVE_TICKS   =  12;   // dash duration (~200 ms at 60 Hz)
inline constexpr int   DASH_COOLDOWN_TICKS =  25;   // cooldown before next dash is allowed (~417 ms)

// Corner correction — player is nudged sideways instead of blocked when overlap ≤ this value
inline constexpr int   CORNER_CORRECTION_PX = TILE_SIZE / 4;  // 8 px

// Dash jump — enhanced jump available in this window immediately after a dash ends
inline constexpr int   DASH_JUMP_WINDOW_TICKS = 10;    // post-dash window (~167 ms)
inline constexpr float DASH_JUMP_FORCE        = 1150.f; // 15% stronger than JUMP_FORCE

// Dash push — force multiplier applied to DASH_SPEED when a dashing player hits another
inline constexpr float DASH_PUSH_MULTIPLIER   = 0.8f;  // pushed player receives reduced dash velocity

// Sprint — held modifier that boosts horizontal movement speed
inline constexpr float SPRINT_MULTIPLIER      = 2.0f;  // 2× faster while sprinting

// Magnet grab — grab and carry a nearby player
inline constexpr float MAGNET_RANGE            = 64.f; // px — max grab radius


// ==========================================================================
// FILE : Player.h
// PATH : src/common/Player.h
// ==========================================================================

﻿#pragma once
#include "PlayerState.h"
#include "InputFrame.h"

// Deterministic player simulation. Owns a PlayerState and updates it each tick.
// Used identically on client (prediction + reconciliation) and server (authority).
class World;

class Player {
public:
    void SetState(const PlayerState& s) { state_ = s; }
    const PlayerState& GetState() const { return state_; }

    // Main update: apply one InputFrame to the current state.
    // Runs all mechanics (movement, coyote, jump, dash, gravity, collision) in fixed order.
    void Simulate(const InputFrame& frame, const World& world);

    // Low-level helpers — exposed for unit tests and split-step internal use.
    void MoveX(float input_dx, const World& world);
    void MoveY(float dt, const World& world);
    void RequestJump();
    void CutJump();    // call when jump key is released while the player is still rising
    void RequestDash(float dx, float dy);
    void SteerDash(float dx, float dy);

    // Reset non-serialised transient state (call on level change).
    void ResetTransient() { prev_jump_held_ = false; }

private:
    PlayerState state_;
    bool        prev_jump_held_ = false;

    void ResolveCollisionsX(const World& world, float dx);
    void ResolveCollisionsY(const World& world);
};


// ==========================================================================
// FILE : PlayerState.h
// PATH : src/common/PlayerState.h
// ==========================================================================

#pragma once
#include <cstdint>

// Plain-data player snapshot. Fully serialisable; shared between client and server.
struct PlayerState {
    // Position (pixels, top-left origin)
    float    x          = 0.f;
    float    y          = 0.f;

    // Velocity
    float    vel_x      = 0.f;   // transient impulse channel (e.g. wall-jump push)
    float    vel_y      = 0.f;
    float    move_vel_x = 0.f;   // inertia-smoothed horizontal speed (px/s)

    // Collision flags — updated each tick by MoveX / MoveY
    bool     on_ground     = false;
    bool     on_wall_left  = false;
    bool     on_wall_right = false;

    // Jump mechanics
    uint8_t  jump_buffer_ticks  = 0;  // > 0: buffered jump awaiting a valid surface
    uint8_t  coyote_ticks       = 0;  // > 0: grace ticks after leaving a ledge
    int8_t   last_wall_jump_dir = 0;  // prevents double-jump on same wall; reset on landing (-1/0/+1)

    // Dash
    uint8_t  dash_active_ticks   = 0;   // > 0: dash in progress (gravity + directional input suspended)
    uint8_t  dash_cooldown_ticks = 0;   // > 0: dash unavailable
    bool     dash_ready          = true; // false after air dash; reset on landing
    int8_t   last_dir            = 1;   // last non-zero horizontal dir; fallback dash target (-1/+1)
    float    dash_dir_x          = 0.f; // normalised current dash direction
    float    dash_dir_y          = 0.f;
    uint8_t  dash_jump_ticks     = 0;   // > 0: post-dash window where jump force is boosted

    // Identity / network
    uint32_t player_id           = 0;
    uint32_t last_processed_tick = 0;   // last tick acknowledged by server; drives client reconciliation

    // Display name (max 15 chars + null terminator)
    char     name[16]            = {};

    // Race timing — level_ticks freezes when finished = true
    uint32_t level_ticks         = 0;
    bool     finished            = false; // true after the player touches an exit tile 'E'

    // Kill tile respawn state
    uint8_t  kill_respawn_ticks  = 0;   // > 0: player is dead (touched 'K'); counts to 0 then respawns
    uint8_t  respawn_grace_ticks = 0;   // > 0: just respawned; input blocked; triggers Ready/Go! overlay on client

    // Checkpoint — updated server-side when the player touches a 'C' tile.
    // (0,0) means no checkpoint has been reached yet (fall back to level spawn).
    float    checkpoint_x = 0.f;
    float    checkpoint_y = 0.f;

    // Drawing trail — true while the player holds the draw button
    bool     drawing      = false;

    // Sprint — true while the player holds the sprint button
    bool     sprinting    = false;

    // Magnet — true while the player holds the magnet button
    bool     magneting    = false;

    // Grabbed — true while another player is carrying this player via magnet
    bool     grabbed      = false;
};

// ==========================================================================
// FILE : Protocol.h
// PATH : src/common/Protocol.h
// ==========================================================================

#pragma once
#include <cstdint>
#include <cstddef>
#include "InputFrame.h"
#include "PlayerState.h"
#include "GameState.h"

// Increment PROTOCOL_VERSION on any breaking change to packet layout, PlayerState,
// or simulation behaviour so client and server can detect incompatibility at connect time.
static constexpr const char*  GAME_VERSION     = "0.2.7";
static constexpr uint16_t     PROTOCOL_VERSION = 12;

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
    PKT_RESTART           = 6,   // C → S  respawn at last shared checkpoint (or spawn if none); Backspace / Triangle
    PKT_LOAD_LEVEL        = 7,   // S → C  load next level or return to menu (is_last=1)
    PKT_VERSION_MISMATCH  = 8,   // S → C  incompatible version; server disconnects immediately after
    PKT_SERVER_BUSY       = 9,   // S → C  session in progress, retry later
    PKT_LEVEL_RESULTS     = 10,  // S → C  end-of-level leaderboard
    PKT_READY             = 11,  // C → S  player ready for the next level
    PKT_GLOBAL_RESULTS    = 12,  // S → C  session-end global leaderboard (wins per player)
    PKT_RESTART_SPAWN     = 13,  // C → S  respawn always at level spawn, ignoring checkpoints; Delete
    PKT_LEVEL_DATA        = 14,  // S → C  generated level tile grid (variable-size packet)
    PKT_EMOTE             = 15,  // C → S  emote selection (emote_id 0-7)
    PKT_EMOTE_BROADCAST   = 16,  // S → C  broadcast emote to all clients
    PKT_GENERATING        = 17,  // S → C  server is generating the next level; client shows loading overlay
    PKT_SET_GAME_MODE     = 18,  // C → S  leader sets the game mode (coop / race)
    PKT_START_GAME        = 19,  // C → S  leader starts the game from the lobby
    PKT_SET_MAX_LEVELS    = 20,  // C → S  leader sets generated levels per session
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
    uint8_t     type              = PKT_LEVEL_RESULTS;
    uint8_t     count             = 0;
    uint8_t     level             = 0;
    uint8_t     coop_all_finished = 0;  // 1 = all players finished (cooperative mode)
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
    uint8_t           total_levels = 0;    // how many levels were played this session
    uint8_t           coop_wins    = 0;    // how many levels the team cleared
    uint8_t           _pad[4]      = {};
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
static constexpr int MAX_GENERATED_LEVELS       = 5;   // default levels per session
static constexpr int MIN_GENERATED_LEVELS       = 1;
static constexpr int MAX_GENERATED_LEVELS_LIMIT = 20;
static constexpr int DIFFICULTY_CURVE_LEVELS = 8;   // difficulty ramp reference

// Emote system — 8 directional emotes (mapped clockwise from Up).
static constexpr int   EMOTE_COUNT      = 8;
static constexpr float EMOTE_DURATION    = 2.0f;   // seconds the bubble is visible
static constexpr const char* EMOTE_TEXTS[EMOTE_COUNT] = { /* body stripped */ }
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

// Leader sets the game mode. Only accepted from the current session leader.
struct PktSetGameMode {
    uint8_t type      = PKT_SET_GAME_MODE;
    uint8_t game_mode = 0;  // 0 = COOP, 1 = RACE, 2 = VERSUS
};

// Leader sets how many generated levels are played in this session.
struct PktSetMaxLevels {
    uint8_t type       = PKT_SET_MAX_LEVELS;
    uint8_t max_levels = static_cast<uint8_t>(MAX_GENERATED_LEVELS);
};

// Leader starts the game from the lobby. Only accepted from the current session leader.
struct PktStartGame {
    uint8_t type = PKT_START_GAME;
};


// ==========================================================================
// FILE : SpawnFinder.h
// PATH : src/common/SpawnFinder.h
// ==========================================================================

#pragma once
// Header-only spawn utility shared by client (GameSession) and server (LevelManager).
// No Raylib or ENet dependency — only World.h and Physics.h.
#include "World.h"
#include "Physics.h"
#include <cstdlib>   // std::abs (int)
#include <climits>   // INT_MAX
#include <vector>
#include <utility>   // std::pair

struct SpawnPos { float x = 0.f, y = 0.f; };

// Returns pixel position (top-left) of the best spawn point among 'X' tiles:
//   x = horizontal centre of all 'X' tiles, snapped to the nearest tile
//   y = lowest 'X' tile that has solid floor directly beneath it
inline SpawnPos FindCenterSpawn(const World& world) { /* body stripped */ }
    // Among 'X' tiles at best_ty, pick the one closest to the horizontal mean.

// Returns the best spawn position for the checkpoint group that contains (ref_tx, ref_ty).
// Flood-fills all 'C' tiles 4-connected to the seed tile, then applies the same
// center + lowest-with-solid-floor logic as FindCenterSpawn.
inline SpawnPos FindCenterCheckpoint(const World& world, int ref_tx, int ref_ty) { /* body stripped */ }
    // Flood-fill via explicit stack (no <stack> needed).
    // Horizontal mean of all tiles in the group.
    // Lowest tile that has solid floor directly beneath it.
    // Fallback: if no tile has solid floor beneath, pick the lowest tile.
    // Among tiles at best_ty, pick the one closest to the horizontal mean.


// ==========================================================================
// FILE : World.h
// PATH : src/common/World.h
// ==========================================================================

#pragma once
#include <string>
#include <vector>

// Tilemap loaded from either:
//   • a plain-text .txt file  (legacy format, kept for unit tests)
//   • a Tiled JSON map   .tmj file  (current format)
//
// Recognised tile chars (stored internally regardless of source format):
//   '0' = wall / platform  (solid)
//   'E' = exit             (non-solid; win condition on touch)
//   'K' = kill             (non-solid by default; touching respawns the player)
//   'X' = spawn            (non-solid; player start position)
//   ' ' = air
//
// For .tmj files the solid flag comes from the TileSet.tsx "solid" property,
// so each tile type can independently be solid or non-solid.
class World {
public:
    // Returns true if the file was opened and parsed successfully.
    // Accepts both .txt (legacy ASCII) and .tmj (Tiled JSON) paths.
    bool LoadFromFile(const char* path);

    // Load from an in-memory char grid. solid_grid is reconstructed as (ch == '0').
    // Used by the chunk-based level generator to load generated levels on both
    // server and client without writing temporary files.
    bool LoadFromGrid(int w, int h, const std::vector<std::string>& rows);

    // Returns true if tile (tx, ty) should block player movement.
    // For .txt: solid iff char == '0'.
    // For .tmj: solid iff the TSX "solid" property is true for that tile type.
    // Out-of-bounds coords always return false.
    bool IsSolid(int tx, int ty) const;

    // Returns the canonical tile char at (tx, ty), or ' ' if out of bounds.
    char GetTile(int tx, int ty) const;

    int GetWidth()  const { return width_; }
    int GetHeight() const { return height_; }

    // Replace all checkpoint tiles ('C') with air (' '). Used in race mode
    // where checkpoints are not part of the gameplay.
    void StripCheckpoints();

    const std::vector<std::string>&       GetRows()      const { return rows_; }
    const std::vector<std::vector<bool>>& GetSolidGrid() const { return solid_grid_; }

private:
    std::vector<std::string>      rows_;        // char grid ('0','E','K','X',' ')
    std::vector<std::vector<bool>> solid_grid_; // parallel solid-flag grid
    int width_  = 0;
    int height_ = 0;

    bool LoadTxt(const char* path);
    bool LoadTmj(const char* path);
};


// ==========================================================================
// FILE : ChunkStore.h
// PATH : src/server/ChunkStore.h
// ==========================================================================

#pragma once
// SRP: loads and classifies all chunk TMJ files from a directory at startup.
// Stores parsed chunks (char grids + metadata) in memory for use by LevelGenerator.
// No ENet or Raylib dependency.

#include "World.h"
#include <string>
#include <vector>
#include <utility>

// One chunk loaded in memory, parsed from a .tmj file.
struct Chunk {
    int width  = 0;
    int height = 0;

    std::vector<std::string>       rows;       // char grid ('0','E','K','X','C','I','O',' ')
    std::vector<std::vector<bool>> solid_grid; // parallel solid-flag grid

    // Primary entry / exit tile positions (-1 if absent).
    // These are the first 'I' and 'O' tiles found during scanning
    // and remain for backward compatibility with single-entry/exit chunks.
    int entry_tx = -1, entry_ty = -1;
    int exit_tx  = -1, exit_ty  = -1;

    // All entry ('I') and exit ('O') tile positions.
    // Used to detect fork chunks: multiple exits = fork start, multiple entries = fork end.
    std::vector<std::pair<int,int>> entries;  // all 'I' tile positions (tx, ty)
    std::vector<std::pair<int,int>> exits;    // all 'O' tile positions (tx, ty)

    // Metadata from TMJ map properties
    std::string role;       // "start", "mid", "end", "any"
    int         difficulty = 1;
    int         weight     = 1;

    // Content flags — set during loading
    bool has_spawn      = false; // contains 'X'
    bool has_end        = false; // contains 'E'
    bool has_checkpoint = false; // contains 'C'

    // Fork detection helpers
    bool IsForkStart() const { return exits.size() > 1; }
    bool IsForkEnd()   const { return entries.size() > 1; }
    int  ExitCount()   const { return static_cast<int>(exits.size()); }
    int  EntryCount()  const { return static_cast<int>(entries.size()); }
};

// Loads all .tmj chunk files from a directory and classifies them into
// start, mid, end, fork_start, and fork_end pools.
class ChunkStore {
public:
    // Scan `dir` for .tmj files, parse each into a Chunk, classify into pools.
    // Returns true if at least one start, one mid, and one end chunk were found.
    bool LoadFromDirectory(const char* dir);

    const std::vector<Chunk>& StartChunks()     const { return start_; }
    const std::vector<Chunk>& MidChunks()       const { return mid_; }
    const std::vector<Chunk>& EndChunks()       const { return end_; }
    const std::vector<Chunk>& ForkStartChunks() const { return fork_start_; }
    const std::vector<Chunk>& ForkEndChunks()   const { return fork_end_; }

    // Min / max difficulty found among mid chunks (for difficulty curve mapping).
    int MinMidDifficulty() const { return min_mid_diff_; }
    int MaxMidDifficulty() const { return max_mid_diff_; }

    bool IsReady() const { return !start_.empty() && !mid_.empty() && !end_.empty(); }

    // True if fork chunks are available (both fork-start and fork-end).
    bool HasForkChunks() const { return !fork_start_.empty(); }

    // Mid-chunk sub-pools based on checkpoint content.
    const std::vector<Chunk>& MidCheckpointChunks() const { return mid_checkpoint_; }
    const std::vector<Chunk>& MidNormalChunks()      const { return mid_normal_; }
    bool HasMidCheckpointChunks() const { return !mid_checkpoint_.empty(); }

private:
    // Parse a single .tmj chunk file. Returns false on failure.
    bool ParseChunk(const char* path, Chunk& out);

    // Classify chunk into start/mid/end pools based on role + content.
    void Classify(Chunk chunk);

    // After all chunks are loaded, scan for fork chunks (multi-exit / multi-entry).
    void DetectForkChunks();

    std::vector<Chunk> start_;
    std::vector<Chunk> mid_;
    std::vector<Chunk> end_;
    std::vector<Chunk> fork_start_;      // chunks with multiple exits (fork entry points)
    std::vector<Chunk> fork_end_;        // chunks with multiple entries (fork merge points)
    std::vector<Chunk> mid_checkpoint_;  // mid chunks that contain 'C' tiles
    std::vector<Chunk> mid_normal_;      // mid chunks without 'C' tiles
    int min_mid_diff_ = 1;
    int max_mid_diff_ = 1;
};


// ==========================================================================
// FILE : LevelGenerator.h
// PATH : src/server/LevelGenerator.h
// ==========================================================================

#pragma once
// SRP: composes a playable level from chunk pieces stored in ChunkStore.
// Algorithm: 1 start + N mid + 1 end, stitched via entry/exit tile alignment.
// Supports branching paths via fork-start (multi-exit) and fork-end (multi-entry) chunks.
// Adds a 5-tile solid border + 5-tile empty margin around the composed map.
// Ensures at least 2 tiles of solid border between parallel branches.
// No ENet or Raylib dependency.

#include "ChunkStore.h"
#include "World.h"
#include "Protocol.h"
#include <cstdint>
#include <random>
#include <unordered_map>

struct GeneratorParams {
    int level_num    = 1;    // current level number (1-based)
    int total_levels = DIFFICULTY_CURVE_LEVELS;  // difficulty curve reference
    uint32_t seed    = 0;    // RNG seed (0 = use current time)
};

class LevelGenerator {
public:
    // Generate a level from the chunks in `store` and load it into `world`.
    // Returns true on success.
    static bool Generate(const ChunkStore& store, const GeneratorParams& params, World& world);

private:
    // Choose how many mid chunks based on level progression.
    static int MidChunkCount(int level_num, int total_levels);

    // Compute the target difficulty [0.0, 1.0] → mapped to the store's range.
    static float TargetDifficulty(int level_num, int total_levels);

    // Difficulty-based branching parameters.
    // Easy (t≈0): more simultaneous paths (4-5), more arrivals (3-4).
    // Hard (t≈1): fewer simultaneous paths (max 2), fewer arrivals (1-2).
    static int MaxSimultaneousPaths(float t);
    static int MaxArrivals(float t);
};


// ==========================================================================
// FILE : LevelManager.h
// PATH : src/server/LevelManager.h
// ==========================================================================

#pragma once
// SRP: loads maps, calculates the optimal spawn point, builds canonical level paths.
// Supports both file-based loading and chunk-based level generation.
// No ENet or session-logic dependency.
#include "World.h"
#include "ChunkStore.h"
#include "LevelGenerator.h"
#include <string>
#include <cstdint>

class LevelManager {
public:
    // Load map from path. Returns false if the file cannot be read.
    bool Load(const char* path);

    // Generate a level from chunks. Returns false on failure.
    // When validate=false, the physics-based level validator is skipped (online mode).
    // total_levels drives the difficulty ramp horizon used by LevelGenerator.
    bool Generate(int level_num, const ChunkStore& store, uint32_t seed = 0,
                  bool validate = true,
                  int total_levels = DIFFICULTY_CURVE_LEVELS);

    // Build the canonical path for a level number (e.g. 2 → "assets/levels/tilemaps/Level02.tmj").
    static std::string BuildPath(int num);

    float        SpawnX()   const { return spawn_x_; }
    float        SpawnY()   const { return spawn_y_; }
    const World& GetWorld() const { return world_; }
    World&       GetWorldMut()    { return world_; }  // mutable access for mode-specific post-processing

private:
    World world_;
    float spawn_x_ = 0.f;
    float spawn_y_ = 0.f;
};


// ==========================================================================
// FILE : LevelValidator.h
// PATH : src/server/LevelValidator.h
// ==========================================================================

#pragma once
// SRP: validates generated levels by simulating an AI agent through them.
// Uses the real Player::Simulate physics to BFS over reachable tile positions.
// Returns true if a path from spawn ('X') to end ('E') exists.
// No ENet or Raylib dependency.

#include "World.h"

class LevelValidator {
public:
    // Returns true if the level has a viable path from spawn to any 'E' tile
    // using the full game physics (jump, dash, wall-jump, dash-jump).
    static bool Validate(const World& world);
};


// ==========================================================================
// FILE : PlayerReset.h
// PATH : src/server/PlayerReset.h
// ==========================================================================

#pragma once
// Header-only helper that removes duplicate 15-line reset blocks from ServerSession
// (kill tile, restart, level-change events all converge here).
#include "PlayerState.h"

// Reset all movement fields and place the player at spawn coordinates.
//   with_kill = true  → kill_respawn_ticks = 60  (death animation, ~1 s wait before control)
//   with_kill = false → respawn_grace_ticks = 25  (triggers Ready/Go! overlay on the client, ~0.4 s)
// Also clears checkpoint_x/checkpoint_y and resets level_ticks (full restart semantics).
inline PlayerState SpawnReset(PlayerState s, float sx, float sy, bool with_kill) { /* body stripped */ }

// Respawn the player at a checkpoint position.
// Unlike SpawnReset, level_ticks keeps accumulating (time penalty only from downtime)
// and the checkpoint itself is preserved.
// with_kill = true  → kill_respawn_ticks = 60 (used by automatic kill-tile death)
// with_kill = false → respawn_grace_ticks = 25 (used by manual restart-at-checkpoint)
inline PlayerState CheckpointReset(PlayerState s, float cx, float cy, bool with_kill) { /* body stripped */ }
    // level_ticks intentionally NOT reset — time keeps running
    // checkpoint coordinates preserved


// ==========================================================================
// FILE : ServerLogic.h
// PATH : src/server/ServerLogic.h
// ==========================================================================

#pragma once
#include <cstdint>
#include <atomic>
#include "GameMode.h"

// Blocking ENet server loop. Caller must call enet_initialize() beforehand.
// Returns only when stop_flag is set to true.
// When skip_lobby is true the server generates level 1 immediately (no lobby).
// initial_mode sets the starting game mode (RACE for offline, COOP for online).
void RunServer(uint16_t port, const char* map_path, std::atomic<bool>& stop_flag,
               bool skip_lobby = false, GameMode initial_mode = GameMode::COOP);


// ==========================================================================
// FILE : ServerSession.h
// PATH : src/server/ServerSession.h
// ==========================================================================

#pragma once
// SRP: game session state machine for the server.
// Manages connected players, lobby / game / results phases, and level progression.
// Socket lifecycle and the ENet service loop live in RunServer (ServerLogic.cpp).
#include "LevelManager.h"
#include "ChunkStore.h"
#include "Player.h"
#include "Protocol.h"
#include "GameMode.h"
#include <enet/enet.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstdint>

class ServerSession {
public:
    // Load the initial map. Check IsReady() after construction.
    // When skip_lobby is true the lobby is skipped: level 1 is generated immediately.
    // initial_mode sets the starting game mode (used by offline → RACE).
    ServerSession(const char* initial_map_path, int initial_level,
                  bool skip_lobby = false, GameMode initial_mode = GameMode::VERSUS);

    bool IsReady() const { return is_ready_; }

    // ENet event handlers — called by RunServer inside the service loop.

    void OnConnect(ENetHost* host, ENetPeer* peer);

    // Returns true when a level change was triggered (caller must break the inner event loop).
    bool OnDisconnect(ENetHost* host, ENetPeer* peer);

    // Returns true when a level change was triggered.
    bool OnReceive(ENetHost* host, ENetPeer* peer, const uint8_t* data, size_t len);

    // Periodic timer checks (results timeout). Call once per outer loop iteration.
    void CheckTimers(ENetHost* host);

private:
    bool HandleInput     (ENetHost* host, ENetPeer* peer, const PktInput& pkt);
    void HandlePlayerInfo(ENetHost* host, ENetPeer* peer, const PktPlayerInfo& pkt);
    void HandleRestart   (ENetPeer* peer);       // respawn at last checkpoint (or spawn)
    void HandleRestartSpawn(ENetPeer* peer);     // respawn always at level spawn
    bool HandleReady     (ENetHost* host, ENetPeer* peer);  // returns true on level change
    void HandleSetGameMode(ENetHost* host, ENetPeer* peer, const PktSetGameMode& pkt);
    void HandleSetMaxLevels(ENetHost* host, ENetPeer* peer, const PktSetMaxLevels& pkt);
    bool HandleStartGame (ENetHost* host, ENetPeer* peer);  // returns true on level change

    // Load next map, reset all players, broadcast new state.
    void DoLevelChange(ENetHost* host);
    // Broadcast PKT_GLOBAL_RESULTS and enter the global-results phase.
    void SendGlobalResults(ENetHost* host);
    // Send PKT_LOAD_LEVEL(is_last=1) then tear down the session.
    void FinishSession(ENetHost* host);
    // Disconnect all peers, reload the lobby (called when is_last).
    void ResetToInitial(ENetHost* host);
    void SendResults   (ENetHost* host, const char* reason);
    void BroadcastGameState(ENetHost* host);
    void BroadcastLevelData(ENetHost* host);      // send PKT_LEVEL_DATA with generated world grid
    void BroadcastGenerating(ENetHost* host);     // send PKT_GENERATING before level generation starts
    void SendLevelDataToPeer(ENetPeer* peer);     // send PKT_LEVEL_DATA to a single peer
    void UpdateZone();
    bool AllInZone()        const;
    uint32_t CountdownTicks() const;
    PlayerState ApplySpawnReset(PlayerState s, bool with_kill) const;
    void ResolvePlayerCollisions(const World& world);   // coop mode: push overlapping player AABBs apart
    void ApplyMagnetGrab(ENetPeer* break_free = nullptr);  // magnet holders grab & carry nearby players
    void ReleaseGrab(ENetPeer* grabber);                 // release a grabbed player (if any)

    // Leader election: elect a new leader from the remaining players.
    void ElectLeader();

    LevelManager level_mgr_;
    ChunkStore   chunk_store_;       // loaded at construction; used by LevelGenerator

    std::string  initial_map_path_;
    int          initial_level_;
    int          current_level_;
    bool         skip_lobby_          = false;
    uint32_t     coop_cleared_levels_ = 0;   // number of levels fully cleared by the team
    uint8_t      session_max_levels_  = static_cast<uint8_t>(MAX_GENERATED_LEVELS);

    bool         is_ready_           = false;
    bool         in_lobby_            = false;
    bool         game_locked_         = false;
    bool         in_results_          = false;
    bool         in_global_results_   = false;  // true while showing session-end global leaderboard

    GameMode     game_mode_           = GameMode::COOP;
    uint32_t     leader_id_           = 0;      // player_id of the current session leader

    uint32_t     session_token_           = 0u;
    uint32_t     next_player_id_          = 1u;
    uint32_t     results_start_ms_        = 0u;
    uint32_t     global_results_start_ms_ = 0u;
    uint32_t     zone_start_ms_           = 0u;
    uint32_t     level_start_ms_          = 0u;

    std::unordered_map<ENetPeer*, Player>   players_;
    std::unordered_map<ENetPeer*, uint32_t> best_ticks_;
    std::unordered_set<ENetPeer*>           ready_peers_;
    // Persistent within a session (survive per-level resets); cleared by ResetToInitial.
    std::unordered_map<uint32_t, uint32_t>  session_wins_;   // player_id → 1st-place count
    std::unordered_map<uint32_t, std::string> session_names_; // player_id → display name

    // Shared checkpoint tracking — each activated spawn-position is recorded so that
    // re-visiting an already-activated checkpoint doesn't reset everyone's progress.
    std::vector<std::pair<float,float>> activated_checkpoints_;

    // Magnet-grab relationships: grabber peer → grabbed peer.
    std::unordered_map<ENetPeer*, ENetPeer*> grab_targets_;
    // Grabbers in this set must release magnet before they can grab again.
    std::unordered_set<ENetPeer*> regrab_requires_release_;

    static constexpr uint32_t LEVEL_TIME_LIMIT_MS        = 120'000u;
    static constexpr uint32_t NEXT_LEVEL_MS              =   3'000u;
    static constexpr uint32_t RESULTS_DURATION_MS        =  15'000u;
    static constexpr uint32_t GLOBAL_RESULTS_DURATION_MS =  25'000u;  // longer: final screen
};


// ==========================================================================
// FILE : Colors.h
// PATH : src/client/Colors.h
// ==========================================================================

#pragma once
#include <raylib.h>

// Centralised colour palette. All client code uses these names instead of inline Color literals
// so the visual style can be changed from one place.
// Naming convention: CLRS_<AREA>_<ROLE>

// Backgrounds
static constexpr Color CLRS_BG_GAME        = {  8,  12,  40, 255};
static constexpr Color CLRS_BG_MENU        = {  5,   5,  15, 255};
static constexpr Color CLRS_BG_OVERLAY     = {  5,   5,  15, 200};  // pause overlay
static constexpr Color CLRS_BG_RESULTS     = {  5,  10,  30, 220};  // results screen overlay
static constexpr Color CLRS_BG_DARK_SEMI   = {  0,   0,   0, 160};  // generic semi-transparent black

// UI — menus, pause, HUD
static constexpr Color CLRS_ACCENT         = {  0, 220, 255, 255};  // cyan — highlighted element
static constexpr Color CLRS_ACCENT_SOFT    = {  0, 220, 255, 200};
static constexpr Color CLRS_TEXT_MAIN      = {200, 200, 220, 255};
static constexpr Color CLRS_TEXT_DIM       = { 80,  80, 100, 255};
static constexpr Color CLRS_TEXT_SOFT_WHITE= {255, 255, 255, 200};
static constexpr Color CLRS_TEXT_HALF_WHITE= {255, 255, 255, 128};

// Player colours — local vs remote, dash ready vs cooling down
static constexpr Color CLRS_PLAYER_LOCAL        = {  0, 220, 255, 255};
static constexpr Color CLRS_PLAYER_LOCAL_DIM    = {  0,  90, 115, 255};  // dash unavailable
static constexpr Color CLRS_PLAYER_REMOTE       = {255, 140,  60, 200};
static constexpr Color CLRS_PLAYER_REMOTE_DIM   = {140,  80,  40, 160};
static constexpr Color CLRS_PLAYER_REMOTE_NAME  = {255, 210, 150, 200};

// Tiles
static constexpr Color CLRS_TILE_WALL       = { 60, 100, 180, 255};  // '0' solid wall
static constexpr Color CLRS_TILE_EXIT       = { 14, 140, 124, 255};  // 'E' exit / win tile
static constexpr Color CLRS_TILE_KILL       = {220,  50,  50, 255};  // 'K' lethal tile
static constexpr Color CLRS_TILE_SPAWN      = {106, 111,  50, 0};  // 'X' spawn point
static constexpr Color CLRS_TILE_CHECKPOINT = { 50, 230,  80, 255};  // 'C' checkpoint — bright green

// HUD timers
static constexpr Color CLRS_TIMER_FINISHED = {  0, 230, 100, 255};  // goal reached
static constexpr Color CLRS_TIMER_BEST     = {  0, 220, 255, 200};  // personal best / level countdown
static constexpr Color CLRS_TIME_LIMIT_WARN= {255,  80,  80, 255};  // time limit critical

// Lobby
static constexpr Color CLRS_LOBBY_HINT     = {200, 200, 220, 220};

// Ready/Go overlay — use .r .g .b with a runtime alpha value; alpha here is ignored
static constexpr Color CLRS_READYGO_READY  = {255, 220,   0, 255};  // yellow — "Ready?"
static constexpr Color CLRS_READYGO_GO     = { 50, 220,  80, 255};  // green  — "Go!"

// End-of-level results screen
static constexpr Color CLRS_RESULTS_TITLE  = { 80, 220,  80, 255};
static constexpr Color CLRS_RESULTS_GOLD   = {255, 215,   0, 255};
static constexpr Color CLRS_RESULTS_SILVER = {200, 200, 200, 255};
static constexpr Color CLRS_RESULTS_BRONZE = {205, 127,  50, 255};
static constexpr Color CLRS_RESULTS_READY  = {100, 220, 100, 255};

// Network stats
static constexpr Color CLRS_STAT_GOOD      = {100, 230, 100, 200};
static constexpr Color CLRS_STAT_OK        = {230, 200,  60, 200};
static constexpr Color CLRS_STAT_BAD       = {230,  80,  80, 200};
static constexpr Color CLRS_STAT_NEUTRAL   = {200, 200, 200, 160};  // jitter (neutral value)

// Error / session end
static constexpr Color CLRS_ERROR          = {255,  80,  80, 255};
static constexpr Color CLRS_SESSION_OK     = { 80, 220,  80, 255};
static constexpr Color CLRS_SESSION_SUB    = {180, 180, 180, 255};

// Debug overlay
static constexpr Color CLRS_DEBUG_TEXT     = {200, 200, 200, 200};


// ==========================================================================
// FILE : GameSession.h
// PATH : src/client/GameSession.h
// ==========================================================================

#pragma once
// Manages one full play session from connect to disconnect.
// Responsibilities: 60 Hz fixed-step physics, network polling, client-side prediction
// and server reconciliation, visual effects, live leaderboard, results screen,
// pause menu, and renderer coordination.
// main.cpp is a thin orchestrator: ShowMainMenu → GameSession → repeat.
#include "World.h"
#include "Player.h"
#include "Protocol.h"
#include "VisualEffects.h"
#include "InputSampler.h"
#include "NetworkClient.h"
#include "SfxManager.h"
#include "SaveData.h"
#include "LevelPalette.h"
#include <raylib.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <cstdint>

class Renderer;

class GameSession {
public:
    struct Config {
        const char* map_path      = nullptr;
        const char* username      = "";
        bool        is_offline    = false;  // true → connects to a LocalServer instance
        SaveData*   save          = nullptr; // for persisting settings (e.g. mute) changed in-session
        int         gamepad_index = -1;     // gamepad claimed at splash screen (-1 = keyboard only)
    };

    explicit GameSession(const Config& cfg);

    // Execute one game-loop iteration (physics + network + render).
    // Returns true while the session is active.
    bool Tick(float dt, NetworkClient& net, Renderer& renderer);

    bool               IsOver()        const { return session_over_; }
    const std::string& GetEndMessage() const { return end_message_; }
    const std::string& GetEndSubMsg()  const { return end_sub_msg_; }
    Color              GetEndColor()   const { return end_color_; }

private:
    World       world_;
    Player      player_;
    uint32_t    local_player_id_ = 0;
    std::string username_;
    bool        is_offline_;
    uint8_t     current_level_ = 0;  // current level number from server (0 = lobby/unknown)
    SaveData*   save_          = nullptr; // non-owning; may be null

    static constexpr uint32_t IHIST = 128;   // input ring-buffer capacity for reconciliation
    InputFrame  input_history_[IHIST] = {};
    GameState   last_game_state_{};
    InputSampler input_sampler_;

    float    accumulator_ = 0.f;
    uint32_t sim_tick_    = 0;
    float    prev_x_      = 0.f;  // position at previous tick (trail / sub-frame interpolation)
    float    prev_y_      = 0.f;

    TrailState   trail_;
    std::unordered_map<uint32_t, TrailState>     remote_trails_;
    std::unordered_map<uint32_t, uint32_t>       remote_last_ticks_;
    DeathParticles                               local_death_{};
    std::unordered_map<uint32_t, DeathParticles> remote_deaths_;
    std::unordered_map<uint32_t, uint8_t>        remote_prev_kill_ticks_;
    std::unordered_map<uint32_t, Vector2>        remote_last_alive_pos_;
    std::unordered_map<uint32_t, uint8_t>        remote_prev_dash_ticks_;
    std::unordered_map<uint32_t, float>          remote_prev_vel_y_;
    std::unordered_map<uint32_t, int8_t>         remote_prev_wall_jump_dir_;
    std::unordered_map<uint32_t, Vector2>        remote_prev_checkpoint_;
    std::unordered_map<uint32_t, bool>           remote_prev_finished_;
    std::unordered_map<uint32_t, bool>           prev_grabbed_state_;

    SfxManager sfx_;
    LevelPalette palette_;  // current level colour theme; default = lobby colours

    std::unordered_map<uint32_t, uint32_t> live_best_ticks_;

    static constexpr double RESULTS_DURATION_S        = 15.0;
    static constexpr double GLOBAL_RESULTS_DURATION_S = 25.0;
    bool        in_results_screen_  = false;
    bool        local_ready_        = false;
    double      results_start_time_ = 0.0;
    uint8_t     results_count_      = 0;
    uint8_t     results_level_      = 0;
    ResultEntry results_entries_[MAX_PLAYERS] = {};

    // Global (session-end) leaderboard state
    bool              in_global_results_screen_  = false;
    bool              local_global_ready_         = false;
    double            global_results_start_time_  = 0.0;
    uint8_t           global_results_count_       = 0;
    uint8_t           global_results_total_levels_= 0;
    uint8_t           global_results_coop_wins_   = 0;   // levels cleared by team
    GlobalResultEntry global_results_entries_[MAX_PLAYERS] = {};

    bool     prev_finished_ = false;
    uint32_t best_ticks_    = 0;
    bool     show_record_   = false;
    bool     results_coop_all_finished_ = false;   // set when PKT_LEVEL_RESULTS arrives in coop mode

    float   last_safe_x_           = 0.f;
    float   last_safe_y_           = 0.f;
    uint8_t prev_kill_ticks_local_ = 0;
    uint8_t prev_grace_local_      = 0;    // tracks respawn_grace_ticks for Ready/Go SFX
    float   prev_checkpoint_x_     = 0.f;  // tracks local checkpoint for SFX
    float   prev_checkpoint_y_     = 0.f;

    PauseState pause_state_    = PauseState::PLAYING;
    int        pause_focused_  = 0;   // 0=Resume, 1=Quit to Menu
    int        confirm_focused_= 0;   // 0=No, 1=Yes
    float      prev_pause_right_stick_x_ = 0.f;

    bool        session_over_ = false;
    std::string end_message_;
    std::string end_sub_msg_;
    Color       end_color_{255, 80, 80, 255};

    // Set by PKT_GENERATING; cleared when PKT_LEVEL_DATA arrives.
    // While true the loading overlay is shown and disconnect-during-gen is suppressed.
    bool    generating_level_     = false;
    uint8_t generating_level_num_ = 0;
    float   generating_elapsed_   = 0.f;

    // Queued disconnect reason received via PKT_VERSION_MISMATCH before the ENet DISCONNECT event.
    std::string pending_disc_reason_;
    std::string pending_disc_sub_;

    // Emote system — per-player bubble state
    std::unordered_map<uint32_t, EmoteBubble> emote_bubbles_;

    // Drawing trails — persistent map marks left by players holding the draw button.
    // Keyed by player_id; each entry is a list of strokes (each stroke = vector of points).
    static constexpr float DRAW_MIN_DIST    = 8.f;   // min px between consecutive points
    static constexpr int   DRAW_MAX_POINTS  = 4000;  // max points per player per level
    static constexpr int   TESS_DIVISIONS   = 6;     // Catmull-Rom subdivisions per segment
    static constexpr float DRAW_LIFETIME_S  = 15.f;  // strokes fade out and expire after this time
    struct DrawStroke {
        std::vector<Vector2> pts;           // raw control points
        std::vector<Vector2> tessellated;   // cached spline polyline
        int tess_source_count = 0;          // # of pts already tessellated
        double created_time_s = 0.0;        // creation timestamp (GetTime)
    };
    std::unordered_map<uint32_t, std::vector<DrawStroke>> draw_trails_;
    std::unordered_map<uint32_t, bool> draw_prev_drawing_;  // previous tick's drawing flag

    void UpdateStrokeTessellation(DrawStroke& st);
    void HandlePauseInput(Renderer& renderer, NetworkClient& net);
    void TickFixed(NetworkClient& net);
    void PollNetwork(NetworkClient& net);
    void HandlePacket(const uint8_t* data, size_t size, NetworkClient& net);
    void HandleDisconnect(uint32_t disconnect_data);
    void LoadLevel(const char* path);
    void LoadLevelFromGrid(int w, int h, const std::vector<std::string>& rows);
    void UpdateLiveBestTicks();
    void BuildLiveLeaderboard(LiveLeaderEntry* out, int& count) const;
    void DoRender(float draw_x, float draw_y, float dt,
                  NetworkClient& net, Renderer& renderer);
};


// ==========================================================================
// FILE : HudCoop.h
// PATH : src/client/HudCoop.h
// ==========================================================================

#pragma once
// HUD rendering for co-op game mode.
#include <raylib.h>
#include <cstdint>

struct PlayerState;

void DrawHudModeCoop(Font& font_hud, const PlayerState& s,
                     uint32_t player_count, bool show_players);


// ==========================================================================
// FILE : HudRace.h
// PATH : src/client/HudRace.h
// ==========================================================================

#pragma once
// HUD rendering for race game mode.
#include <raylib.h>
#include <cstdint>

struct PlayerState;

void DrawHudModeRace(Font& font_hud, const PlayerState& s,
                     uint32_t player_count, bool show_players);


// ==========================================================================
// FILE : HudVersus.h
// PATH : src/client/HudVersus.h
// ==========================================================================

#pragma once
// HUD rendering for versus game mode.
#include <raylib.h>
#include <cstdint>

struct PlayerState;

void DrawHudModeVersus(Font& font_hud, const PlayerState& s,
                       uint32_t player_count, bool show_players);


// ==========================================================================
// FILE : InputSampler.h
// PATH : src/client/InputSampler.h
// ==========================================================================

#pragma once
// Centralised keyboard + gamepad sampling.
//
// Per-frame usage:
//   1. Call Poll() once per render frame — captures rising-edge events.
//   2. Call ConsumeXxx() to read sticky flags; each returns true exactly once.
//   3. Call GetMoveX() / GetDashDir() / IsJumpHeld() inside each fixed tick
//      (they read live hardware state on every call).
//
// Gamepad claim:
//   gp_index_ starts at -1 (unclaimed). The first gamepad button pressed during
//   Poll() automatically claims that gamepad for the lifetime of this instance.
//   SetGamepadIndex() allows the caller to pre-assign the index (e.g. from the
//   splash screen, where the user's first button press is detected before
//   GameSession is constructed).
#include <raylib.h>
#include "InputFrame.h"

class InputSampler {
public:
    // Capture all IsKeyPressed / IsGamepadButtonPressed for this render frame.
    void Poll();

    // Gamepad claim management.
    // Returns -1 if no gamepad has been claimed yet.
    int  GetGamepadIndex() const { return gp_index_; }
    // Pre-assign the gamepad index (e.g. from splash screen claim).
    // Has no effect if idx < 0 or if a gamepad has already been claimed.
    void SetGamepadIndex(int idx) { if (idx >= 0 && gp_index_ < 0) gp_index_ = idx; }

    // Sticky flags — return true once then reset to false.
    bool ConsumeJumpPressed()        { bool v = jump_pressed_;           jump_pressed_           = false; return v; }
    bool ConsumeDashPending()        { bool v = dash_pending_;           dash_pending_           = false; return v; }
    bool ConsumePauseToggle()        { bool v = pause_toggle_;           pause_toggle_           = false; return v; }
    // Restart from last shared checkpoint (or spawn if none): Backspace / Triangle
    bool ConsumeRestartRequest()     { bool v = restart_checkpoint_;     restart_checkpoint_     = false; return v; }
    // Restart always from level spawn, clearing checkpoint: Delete
    bool ConsumeRestartSpawn()       { bool v = restart_spawn_;          restart_spawn_          = false; return v; }

    // Pause menu navigation — refreshed each render frame; do not consume.
    bool PauseNavUp()   const { return nav_up_;   }
    bool PauseNavDown() const { return nav_down_; }
    bool PauseNavOk()   const { return nav_ok_;   }

    // Live hardware state — safe to call every fixed tick.
    float GetMoveX()                       const;
    void  GetDashDir(float& dx, float& dy) const;
    bool  IsJumpHeld()                     const;
    bool  IsDrawHeld()                      const;
    bool  IsSprintHeld()                     const;
    bool  IsMagnetHeld()                     const;

    // Emote wheel (E key / right-stick click).
    bool IsEmoteWheelOpen()       const { return emote_wheel_open_; }
    int  GetEmoteWheelHighlight() const { return emote_highlight_; }  // -1 = none, 0-7 = direction
    int  ConsumeEmotePending()           { int v = emote_pending_; emote_pending_ = -1; return v; }

private:
    static constexpr float GP_DEADZONE = 0.25f;
    static constexpr int   GP_MAX      = 4;  // max gamepads to scan for auto-claim

    int   gp_index_           = -1;   // -1 = unclaimed; set on first gamepad button press

    bool  jump_pressed_       = false;
    bool  dash_pending_       = false;
    bool  restart_checkpoint_ = false;  // Backspace / Triangle
    bool  restart_spawn_      = false;  // Delete
    bool  pause_toggle_       = false;

    bool  nav_up_   = false;
    bool  nav_down_ = false;
    bool  nav_ok_   = false;

    float prev_stick_y_ = 0.f;  // previous-frame stick Y for edge detection in pause navigation

    // Emote wheel state
    bool  emote_wheel_open_ = false;  // true while activation key is held
    int   emote_highlight_  = -1;     // currently highlighted direction (-1 = none)
    int   emote_pending_    = -1;     // emote to send (-1 = none)
    bool  emote_kb_was_open_= false;  // previous-frame E key state (edge detection)
    bool  emote_gp_was_open_= false;  // previous-frame L1 state

    void PollEmote();  // called by Poll()
};


// ==========================================================================
// FILE : LevelPalette.h
// PATH : src/client/LevelPalette.h
// ==========================================================================

#pragma once
// LevelPalette — per-level world colour theme derived from a level number.
//
// Only the world/environment colours change; player colours stay fixed so every
// player always recognises their own character regardless of the current level.
//
// Generation algorithm:
//   base_hue = level_num * GOLDEN_ANGLE_DEG  (mod 360)
//              → 8 levels spread maximally across the hue wheel
//   Background : same hue, S=0.65, L=0.08   (almost black — guarantees player contrast)
//   Wall       : same hue, S=0.45, L=0.42   (ΔL≈34% vs BG → always readable)
//   Exit       : base_hue +120°, S=0.65, L=0.38  (triadic — always distinct from wall)
//   Kill       : base_hue +180°, S=0.85, L=0.50  (complementary — always distinct)
//   Checkpoint : base_hue +100°, S=0.70, L=0.50  (near-triadic — distinct from kill/wall)
//   Spawn      : transparent (invisible in-game, kept for completeness)
//
// level_num == 0  →  returns the lobby default palette (original Colors.h values).
//
// Header-only; no .cpp required.  Depends only on <raylib.h> and <cmath>.
#include <raylib.h>
#include <cmath>

struct LevelPalette {
    Color bg;
    Color wall;
    Color exit_tile;
    Color kill_tile;
    Color checkpoint;
    Color spawn;  // typically transparent / invisible

    // Default constructor = lobby palette (original Colors.h values).
    // Used for file-based levels and the lobby map.
    LevelPalette()
        : bg        {  8,  12,  40, 255}
        , wall      { 60, 100, 180, 255}
        , exit_tile { 14, 140, 124, 255}
        , kill_tile {220,  50,  50, 255}
        , checkpoint{ 50, 230,  80, 255}
        , spawn     {106, 111,  50,   0}
    {}
};

namespace detail {

// h in [0, 360), s and l in [0, 1]  →  Color {r, g, b, alpha}
inline Color HslToColor(float h, float s, float l, uint8_t alpha = 255) {
    h = std::fmod(h, 360.f);
    if (h < 0.f) h += 360.f;
    const float c  = (1.f - std::fabs(2.f * l - 1.f)) * s;
    const float x  = c * (1.f - std::fabs(std::fmod(h / 60.f, 2.f) - 1.f));
    const float m  = l - c * 0.5f;
    float r = 0.f, g = 0.f, b = 0.f;
    if      (h <  60.f) { r = c; g = x; }
    else if (h < 120.f) { r = x; g = c; }
    else if (h < 180.f) {        g = c; b = x; }
    else if (h < 240.f) {        g = x; b = c; }
    else if (h < 300.f) { r = x;        b = c; }
    else                { r = c;        b = x; }
    return { static_cast<uint8_t>((r + m) * 255.f),
             static_cast<uint8_t>((g + m) * 255.f),
             static_cast<uint8_t>((b + m) * 255.f),
             alpha };
}

} // namespace detail

// Returns the palette for a given level number (1-based).
// level_num == 0 returns the default lobby palette (same as LevelPalette{}).
inline LevelPalette MakeLevelPalette(uint8_t level_num) { /* body stripped */ }
    // Golden-angle distribution: maximally separates up to ~16 distinct hues.
    // Exit and checkpoint share the same visual role ("reach me") — same color.
    // Kill tile stays fixed red regardless of hue — universal danger signal.


// ==========================================================================
// FILE : LevelResultsCoop.h
// PATH : src/client/LevelResultsCoop.h
// ==========================================================================

#pragma once
// End-of-level results screen for co-op mode.
#include <raylib.h>
#include <cstdint>

struct ResultEntry;

void DrawLevelResultsModeCoop(Font& font_hud, Font& font_timer,
                              bool in_results, bool local_ready,
                              const ResultEntry* entries, uint8_t count, uint8_t level,
                              double elapsed_since_start, double total_duration,
                              bool coop_all_finished);


// ==========================================================================
// FILE : LevelResultsRace.h
// PATH : src/client/LevelResultsRace.h
// ==========================================================================

#pragma once
// End-of-level results screen for race mode.
#include <raylib.h>
#include <cstdint>

struct ResultEntry;

void DrawLevelResultsModeRace(Font& font_hud, Font& font_timer,
                              bool in_results, bool local_ready,
                              const ResultEntry* entries, uint8_t count, uint8_t level,
                              double elapsed_since_start, double total_duration,
                              bool coop_all_finished);


// ==========================================================================
// FILE : LocalServer.h
// PATH : src/client/LocalServer.h
// ==========================================================================

#pragma once
#include <cstdint>
#include <atomic>
#include <thread>
#include <string>
#include "GameMode.h"

// Runs a game server instance in a background thread within the same process.
// Used for offline mode: the client connects to 127.0.0.1 via ENet as normal
// without needing a separate TileRace_Server executable.
class LocalServer {
public:
    LocalServer()  = default;
    ~LocalServer() { Stop(); }

    LocalServer(const LocalServer&)            = delete;
    LocalServer& operator=(const LocalServer&) = delete;

    // Start the server on the given port. Blocks ~200 ms waiting for the ENet bind.
    // When skip_lobby is true the lobby is skipped and level 1 is generated immediately.
    // initial_mode sets the starting game mode (RACE for offline).
    void Start(uint16_t port, const char* map_path, bool skip_lobby = false,
               GameMode initial_mode = GameMode::COOP);

    // Signal stop and join the background thread.
    void Stop();

    bool     IsRunning() const { return running_; }
    uint16_t GetPort()   const { return port_; }

private:
    std::thread       thread_;
    std::atomic<bool> stop_flag_{false};
    uint16_t          port_    = 0;
    bool              running_ = false;
};


// ==========================================================================
// FILE : MainMenu.h
// PATH : src/client/MainMenu.h
// ==========================================================================

#pragma once
#include <raylib.h>
#include <cstdint>
#include "SaveData.h"

enum class MenuChoice { OFFLINE, ONLINE, QUIT };

struct MenuResult {
    MenuChoice choice        = MenuChoice::OFFLINE;
    char       server_ip[64] = "127.0.0.1";
    char       username[16]  = "Player";
    uint16_t   port          = 7777;
};

// Blocking splash screen — shows title + "Press any button to start".
// Returns when any key, mouse button, or gamepad button is pressed.
// Returns the index of the gamepad that was pressed (0-based), or -1 if a
// keyboard/mouse input triggered the dismiss (no gamepad claimed).
// The window must already be open (InitWindow already called).
int ShowSplashScreen(Font& font);

// Blocking menu loop — returns only when the user confirms a choice.
// The window must already be open (InitWindow already called).
// Reads initial field values from `save` and writes them back before returning.
// gamepad_index: the gamepad claimed at splash screen time (-1 = keyboard only).
MenuResult ShowMainMenu(Font& font, SaveData& save, int gamepad_index);


// ==========================================================================
// FILE : NetworkClient.h
// PATH : src/client/NetworkClient.h
// ==========================================================================

#pragma once
// ENet abstraction for the client side.
// <enet/enet.h> is NOT included here — the ENet dependency is fully contained in NetworkClient.cpp.
// Replacing ENet requires rewriting only that .cpp file.
#include <cstdint>
#include <cstddef>
#include <vector>

enum class NetEventType { None, Disconnected, Packet };

struct NetEvent {
    NetEventType         type            = NetEventType::None;
    std::vector<uint8_t> data;            // valid when type == Packet
    uint32_t             disconnect_data = 0; // valid when type == Disconnected
};

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    NetworkClient(const NetworkClient&)            = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    // Blocking connect (max `attempts` × 50 ms). Returns false on timeout.
    bool Connect(const char* ip, uint16_t port, int attempts = 60);

    void Disconnect();

    bool IsConnected() const { return peer_ != nullptr; }

    void SendReliable  (const void* data, size_t size);
    void SendUnreliable(const void* data, size_t size);

    // Increase the ENet peer timeout so the connection survives server-side operations
    // that temporarily stall the ENet loop (e.g. level generation).
    // timeout_ms: max milliseconds before hard-disconnect (default ENet ≈ 5 s).
    void SetLongTimeout(uint32_t timeout_ms = 60000);

    // Returns the next queued event. Call in a loop until type == None.
    // Packet bytes are already copied; the underlying ENet packet is destroyed internally.
    NetEvent Poll();

    // Network stats — all zero when not connected.
    uint32_t GetRTT()    const;
    uint32_t GetJitter() const;
    uint32_t GetLoss()   const;

private:
    void* host_ = nullptr;  // ENetHost* — opaque in this header
    void* peer_ = nullptr;  // ENetPeer* — opaque in this header
};


// ==========================================================================
// FILE : Renderer.h
// PATH : src/client/Renderer.h
// ==========================================================================

#pragma once
// Owns all Raylib draw calls.
// Rule: no file other than Renderer.cpp may call DrawXxx, BeginDrawing, BeginMode2D, etc.
// Replacing Raylib requires rewriting only Renderer.cpp.
#include <raylib.h>
#include <cstdint>
#include "VisualEffects.h"
#include "GameState.h"
#include "GameMode.h"
#include "Protocol.h"   // EMOTE_TEXTS, EMOTE_COUNT
#include "LevelPalette.h"

class  World;
struct PlayerState;
struct ResultEntry;
struct GlobalResultEntry;

class Renderer {
public:
    void Init();
    void Cleanup();
    Font& HudFont();   // exposed for ShowMainMenu (the only external consumer)

    // Camera
    void ResetCamera(float cx, float cy);
    void UpdateCamera(float cx, float cy, float dt);
    void SnapCamera(float cx, float cy);
    void TriggerShake(float duration);
    void TickShake(float dt);

    // Frame lifecycle
    void BeginFrame();
    void EndFrame();
    void BeginWorldDraw();   // enters Camera2D mode
    void EndWorldDraw();     // exits Camera2D mode

    // World-space draw calls — must be between BeginWorldDraw / EndWorldDraw
    void DrawTilemap(const World& world);
    void DrawTrail(const TrailState& t, bool is_local);
    void DrawDeathParticles(const DeathParticles& dp);
    void DrawPlayer(float rx, float ry, const PlayerState& s, bool is_local = true,
                    bool is_leader = false);
    void DrawGrabLinkMarker(float ax, float ay, float bx, float by, Color color, float radius);

    // Drawing trails — persistent spline marks left on the map by the draw button.
    // Each stroke is a vector of points; drawn as Catmull-Rom splines.
    struct DrawStroke { const Vector2* pts; int count; };
    void DrawMapTrails(const DrawStroke* strokes, int stroke_count, Color color);

    // Emote system (world-space, call between BeginWorldDraw / EndWorldDraw)
    void DrawEmoteWheel(float cx, float cy, int highlighted_dir);  // local-only wheel overlay
    void DrawEmoteBubble(float px, float py, uint8_t emote_id, float alpha, bool is_local);

    // Screen-space HUD
    void DrawHUD(const PlayerState& s, uint32_t player_count, bool show_players,
                 GameMode mode = GameMode::COOP);
    void DrawLevelIndicator(uint8_t level);  // bottom-center level number
    void DrawNetStats(uint32_t rtt, uint32_t jitter, uint32_t loss_pct);
    void DrawTimer(const PlayerState& s,
                   uint32_t best_ticks, uint32_t time_limit_secs,
                   uint32_t next_level_cd_ticks,
                   GameMode mode = GameMode::COOP);
    void DrawLiveLeaderboard(const LiveLeaderEntry* entries, int count);
    void DrawNewRecord(bool show, bool is_lobby);
    void DrawLobbyHints(uint32_t cd_ticks, uint32_t player_count);

    // Lobby options panel: shows game mode + leader controls.
    void DrawLobbyOptions(GameMode mode, bool is_leader, uint32_t leader_id,
                          const GameState& gs);

    // Off-screen player indicators: orange dot + name on the viewport border (~64 px margin).
    // Call after EndWorldDraw, before any other HUD element.
    void DrawOffscreenArrows(const GameState& gs, uint32_t local_player_id);

    // Ready? / Go! animated overlay.
    // Call every frame with the current respawn_grace_ticks value and dt.
    // Phase transitions are managed internally.
    void DrawReadyGo(uint8_t grace_ticks, float dt);
    void ResetReadyGo();   // call on every level or session load

    void DrawDebugPanel(const PlayerState& s);

    // Level colour palette — call once per level load, before the first BeginFrame.
    // Affects ClearBackground and all tile colours in DrawTilemap.
    void SetPalette(const LevelPalette& p);

    // Pause menu
    Rectangle GetPauseItemRect(int item_index, int total_items = 3) const;  // for mouse hit-testing in GameSession
    void DrawPauseMenu(PauseState state, int focused, int confirm_focused, bool sfx_muted,
                       bool show_lobby_settings = false, GameMode lobby_mode = GameMode::COOP,
                       uint8_t lobby_max_levels = static_cast<uint8_t>(MAX_GENERATED_LEVELS));

    // End-of-level results screen
    void DrawResultsScreen(bool in_results, bool local_ready,
                           const ResultEntry* entries, uint8_t count, uint8_t level,
                           double elapsed_since_start, double total_duration,
                           bool coop_all_finished,
                           GameMode mode = GameMode::COOP);

    // Session-end global leaderboard (shown after the last level).
    void DrawGlobalResultsScreen(bool in_global, bool local_ready,
                                 const GlobalResultEntry* entries, uint8_t count,
                                 uint8_t total_levels,
                                 double elapsed_since_start, double total_duration,
                                 uint8_t coop_wins,
                                 GameMode mode = GameMode::COOP);

    // Full-screen error / session-end overlays used in mini-loops inside main.cpp
    void DrawConnectionErrorScreen(const char* msg);
    void DrawSessionEndScreen(const char* main_msg, const char* sub_msg, Color col);

    // Loading overlay shown while the server generates a level.
    // Call every frame until PKT_LEVEL_DATA arrives.
    void DrawGeneratingLevel(uint8_t level_num, float elapsed_secs);

private:
    Font     font_small_ = {};
    Font     font_hud_   = {};
    Font     font_timer_ = {};
    Font     font_bold_  = {};   // loaded at 200 px — all draw sizes are downscales, no blur
    Camera2D camera_     = {};
    float    cam_shake_timer_ = 0.f;

    LevelPalette palette_;  // current level colour theme; updated via SetPalette()

    enum class ReadyGoPhase { NONE, READY, GO };
    ReadyGoPhase rg_phase_      = ReadyGoPhase::NONE;
    float        rg_go_timer_   = 0.f;   // seconds elapsed in the GO phase
    uint8_t      rg_prev_grace_ = 0;     // grace_ticks from the previous frame (detects 0→>0 transition)
};


// ==========================================================================
// FILE : SaveData.h
// PATH : src/client/SaveData.h
// ==========================================================================

#pragma once
#include <cstdint>

// Persistent player data. Serialised as JSON then XOR-obfuscated before writing to disk.
// Callers work only with this struct; Load/Save handle all I/O.
struct SaveData {
    char username[16] = {};
    char last_ip[64]  = {};
    bool sfx_muted    = false;
};

// Load from "save.dat" in the CWD. Returns true if the file was valid.
// On error (missing file, bad checksum) the struct is left zero-initialised.
bool LoadSaveData(SaveData& out);

void SaveSaveData(const SaveData& data);


// ==========================================================================
// FILE : SessionResultsCoop.h
// PATH : src/client/SessionResultsCoop.h
// ==========================================================================

#pragma once
// Session-end global results screen for co-op mode.
#include <raylib.h>
#include <cstdint>

struct GlobalResultEntry;

void DrawSessionResultsModeCoop(Font& font_hud, Font& font_timer,
                                bool in_global, bool local_ready,
                                const GlobalResultEntry* entries, uint8_t count,
                                uint8_t total_levels,
                                double elapsed_since_start, double total_duration,
                                uint8_t coop_wins);


// ==========================================================================
// FILE : SessionResultsRace.h
// PATH : src/client/SessionResultsRace.h
// ==========================================================================

#pragma once
// Session-end global results screen for race mode.
#include <raylib.h>
#include <cstdint>

struct GlobalResultEntry;

void DrawSessionResultsModeRace(Font& font_hud, Font& font_timer,
                                bool in_global, bool local_ready,
                                const GlobalResultEntry* entries, uint8_t count,
                                uint8_t total_levels,
                                double elapsed_since_start, double total_duration,
                                uint8_t coop_wins);


// ==========================================================================
// FILE : SfxManager.h
// PATH : src/client/SfxManager.h
// ==========================================================================

#pragma once
// SfxManager — owns all SoundPools and one-shot sounds for the game.
// Loads all assets once during GameSession construction; unloads on destruction.
// Play*()   methods: local player (full volume, centre pan).
// Play*At() methods: remote players (2-D spatialized).
#include "SoundPool.h"
#include <raylib.h>

class SfxManager {
public:
    SfxManager();
    ~SfxManager();

    // No copy.
    SfxManager(const SfxManager&)            = delete;
    SfxManager& operator=(const SfxManager&) = delete;

    void SetMuted(bool muted) { muted_ = muted; }
    bool IsMuted()      const { return muted_; }

    // --- Movement sounds ---
    // Local
    void PlayJump();
    void PlayWallJump();
    void PlayDash();
    // Remote (spatialized)
    void PlayJumpAt    (float sx, float sy, float lx, float ly);
    void PlayWallJumpAt(float sx, float sy, float lx, float ly);
    void PlayDashAt    (float sx, float sy, float lx, float ly);

    // --- Death sounds ---
    void PlayDeath();
    void PlayDeathAt(float sx, float sy, float lx, float ly);

    // --- Checkpoint ---
    void PlayCheckpoint();
    void PlayCheckpointAt(float sx, float sy, float lx, float ly);

    // --- Level complete ---
    void PlayLevelEnd();
    void PlayLevelEndAt(float sx, float sy, float lx, float ly);

    // --- UI / level events (local only, no spatialization) ---
    void PlayReady();
    void PlayGo();
    void PlayGrabOn();
    void PlayGrabOff();
    void PlayGrabOnAt(float sx, float sy, float lx, float ly);
    void PlayGrabOffAt(float sx, float sy, float lx, float ly);

private:
    SoundPool jump_pool_;      // 3 variants
    SoundPool wall_jump_pool_; // 3 variants
    SoundPool dash_pool_;      // 3 variants
    SoundPool death_pool_;     // 3 variants
    SoundPool checkpoint_pool_;// 1 variant
    Sound     ready_sound_    = {};
    Sound     go_sound_       = {};
    Sound     level_end_sound_= {};
    Sound     grab_on_sound_  = {};
    Sound     grab_off_sound_ = {};
    bool      muted_ = false;
};


// ==========================================================================
// FILE : SoundPool.h
// PATH : src/client/SoundPool.h
// ==========================================================================

#pragma once
// SoundPool — pool of N audio variants for the same effect.
// Picks a random variant and applies a slight pitch randomisation on every play.
// PlayAt() additionally maps world-space distance to volume attenuation
// and horizontal offset to stereo pan (L/R).
// No external dependencies beyond Raylib.
#include <raylib.h>
#include <vector>

class SoundPool {
public:
    // Load all N sound files whose paths are listed in [paths, paths+count).
    // Call once at startup (after InitAudioDevice).
    void Load(const char* const* paths, int count);

    // Unload all loaded sounds. Call before CloseAudioDevice.
    void Unload();

    // Play a random variant at full volume, centre pan.
    // pitch_variance: ± range around 1.0 (e.g. 0.07 → pitch in [0.93, 1.07]).
    void Play(float pitch_variance = 0.07f);

    // Play with 2-D spatial audio.
    //   source_x / source_y  : world-space pixel position of the emitter.
    //   listener_x / listener_y: world-space pixel position of the listener (camera target).
    //   max_dist             : distance (px) at which volume hits 0.
    void PlayAt(float source_x, float source_y,
                float listener_x, float listener_y,
                float max_dist       = 1200.f,
                float pitch_variance = 0.07f);

private:
    std::vector<Sound> sounds_;

    // Returns the next sound to play (round-robin through the pool so concurrent
    // calls on different remote players don't stomp each other's pitch/pan state).
    Sound& NextVoice();
    int next_idx_ = 0;
};


// ==========================================================================
// FILE : UIWidgets.h
// PATH : src/client/UIWidgets.h
// ==========================================================================

#pragma once
// Stateless Raylib UI helpers — text rendering, buttons, text fields.
// No internal state. Depends only on Raylib and Colors.h.
#include <raylib.h>

namespace UIWidgets {

bool Hovered(Rectangle r);
bool Clicked(Rectangle r);

// Centred text button. Returns its Rectangle for hit-testing.
// selected=true adds "> " prefix and uses CLRS_ACCENT.
Rectangle DrawTextBtn(Font& f, const char* label, float cx, float y, bool selected = false);

// Text input field with label, value, and blinking cursor.
void DrawTextField(Font& f, const char* label,
                   float cx, float y, const char* buf, bool focused);

// Handle keyboard input for a text field (backspace + GetCharPressed).
// Returns true if the buffer was modified.
bool PollFieldInput(char* buf, int maxlen);

// Text centred horizontally on cx.
void DrawCentered(Font& f, const char* text, float cx, float y, float sz, Color col);

} // namespace UIWidgets


// ==========================================================================
// FILE : VisualEffects.h
// PATH : src/client/VisualEffects.h
// ==========================================================================

#pragma once
// Client-only visual effect data structures.
// No Renderer or ENet dependency — only Raylib (Color, Vector2) and Physics.h constants.
#include <raylib.h>
#include <cstdint>

// Dash trail — ring buffer of recent player positions, rendered as fading segments.
struct TrailState {
    struct Point { float x, y; };
    static constexpr int LEN = 12;
    Point pts[LEN] = {};
    int   count    = 0;

    void Push(float x, float y) {
        for (int i = LEN - 1; i > 0; i--) pts[i] = pts[i - 1];
        pts[0] = {x, y};
        if (count < LEN) count++;
    }
    void Clear() { count = 0; }
};

// Death burst particles — spawned the frame kill_respawn_ticks transitions from 0 to > 0.
struct DeathParticle { float x, y, vx, vy, life; };

struct DeathParticles {
    static constexpr int MAX = 18;
    DeathParticle parts[MAX] = {};
    Color         col        = {};

    void Spawn(float cx, float cy, Color c);
    void Update(float dt);
    bool Active() const;
};

// Live in-game leaderboard entry.
struct LiveLeaderEntry {
    uint32_t player_id  = 0;
    uint32_t best_ticks = 0;
    char     name[16]   = {};
};

enum class PauseState { PLAYING, PAUSED, CONFIRM_QUIT, LOBBY_SETTINGS };

// Emote bubble — shown above a player after they trigger an emote.
struct EmoteBubble {
    uint8_t emote_id = 0;      // 0-7 index into EMOTE_TEXTS
    float   timer    = 0.f;    // counts down from EMOTE_DURATION to 0
    bool    active   = false;
    float   last_x   = 0.f;    // last known world position (frozen on death)
    float   last_y   = 0.f;
};


// ==========================================================================
// FILE : WinIcon.h
// PATH : src/client/WinIcon.h
// ==========================================================================

#pragma once
// Sets the embedded icon (resource ID 1 from the .rc file) on the native Win32 window.
// Applies both ICON_SMALL and ICON_BIG so the taskbar and Alt+Tab show the correct icon
// after GLFW/Raylib has initialised the window.
// hwnd_ptr: value returned by Raylib's GetWindowHandle(), cast to void*.
// No-op on non-Windows platforms.
#ifdef _WIN32
void ApplyExeIconToWindow(void* hwnd_ptr);
#else
inline void ApplyExeIconToWindow(void*) {}
#endif

