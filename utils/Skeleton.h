/*
 * ============================================================================
 * SKELETON.H  —  AI Context Snapshot for TileRace
 * Generated : 2026-03-03 08:00
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
 *   │   ├── InputSampler.cpp
 *   │   ├── InputSampler.h
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
 *   │   ├── UIWidgets.cpp
 *   │   ├── UIWidgets.h
 *   │   ├── VisualEffects.cpp
 *   │   ├── VisualEffects.h
 *   │   ├── WinIcon.cpp
 *   │   └── WinIcon.h
 *   ├── common
 *   │   ├── CMakeLists.txt
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
 *   │   ├── CMakeLists.txt
 *   │   ├── LevelManager.cpp
 *   │   ├── LevelManager.h
 *   │   ├── main.cpp
 *   │   ├── PlayerReset.h
 *   │   ├── ServerLogic.cpp
 *   │   ├── ServerLogic.h
 *   │   ├── ServerSession.cpp
 *   │   └── ServerSession.h
 *   └── app_icon.rc.in
 *
 * HEADERS INCLUDED BELOW  (23 files)
 *   [01]  src\common\GameState.h
 *   [02]  src\common\InputFrame.h
 *   [03]  src\common\Physics.h
 *   [04]  src\common\Player.h
 *   [05]  src\common\PlayerState.h
 *   [06]  src\common\Protocol.h
 *   [07]  src\common\SpawnFinder.h
 *   [08]  src\common\World.h
 *   [09]  src\server\LevelManager.h
 *   [10]  src\server\PlayerReset.h
 *   [11]  src\server\ServerLogic.h
 *   [12]  src\server\ServerSession.h
 *   [13]  src\client\Colors.h
 *   [14]  src\client\GameSession.h
 *   [15]  src\client\InputSampler.h
 *   [16]  src\client\LocalServer.h
 *   [17]  src\client\MainMenu.h
 *   [18]  src\client\NetworkClient.h
 *   [19]  src\client\Renderer.h
 *   [20]  src\client\SaveData.h
 *   [21]  src\client\UIWidgets.h
 *   [22]  src\client\VisualEffects.h
 *   [23]  src\client\WinIcon.h
 * ============================================================================
 */


// ==========================================================================
// FILE : GameState.h
// PATH : src/common/GameState.h
// ==========================================================================

#pragma once
#include "PlayerState.h"
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
};


// ==========================================================================
// FILE : InputFrame.h
// PATH : src/common/InputFrame.h
// ==========================================================================

#pragma once
#include <cstdint>

// Button bitmask. Rising-edge flags (JUMP_PRESS, DASH) are set in InputSampler::Poll()
// and cleared after one fixed tick so they fire exactly once per physical press.
enum InputBits : uint8_t {
    BTN_LEFT       = 1 << 0,  // move left (held)
    BTN_RIGHT      = 1 << 1,  // move right (held)
    BTN_JUMP       = 1 << 2,  // jump held; used for variable-height cut
    BTN_JUMP_PRESS = 1 << 3,  // rising edge — sets jump buffer
    BTN_DASH       = 1 << 4,  // rising edge — starts dash
    BTN_UP         = 1 << 5,  // held — dash direction / upward steer
    BTN_DOWN       = 1 << 6,  // held — dash direction / downward steer
};

// Deterministic input snapshot for one 60 Hz fixed tick.
// Identical layout on client and server: same inputs + same PlayerState = same result.
struct InputFrame {
    uint32_t tick    = 0;     // monotonic fixed-tick counter (60 Hz)
    uint8_t  buttons = 0;     // OR of InputBits

    // Raw direction for dash targeting and in-flight steering.
    // Normalised internally in Player::Simulate / RequestDash / SteerDash.
    float dash_dx = 0.f;
    float dash_dy = 0.f;

    // Horizontal movement axis [-1, 1]. Keyboard/DPAD = ±1.0; analogue stick = raw after deadzone.
    float move_x  = 0.f;

    bool Has(InputBits b) const { return (buttons & b) != 0; }
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
inline constexpr float MOVE_SPEED  = 400.f;    // px/s  — max horizontal speed
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
    float    checkpoint_y = 0.f;};

// ==========================================================================
// FILE : Protocol.h
// PATH : src/common/Protocol.h
// ==========================================================================

#pragma once
#include <cstdint>
#include "InputFrame.h"
#include "PlayerState.h"
#include "GameState.h"

// Increment PROTOCOL_VERSION on any breaking change to packet layout, PlayerState,
// or simulation behaviour so client and server can detect incompatibility at connect time.
static constexpr const char*  GAME_VERSION     = "0.1.5b";
static constexpr uint16_t     PROTOCOL_VERSION = 3;

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

    // Returns true if tile (tx, ty) should block player movement.
    // For .txt: solid iff char == '0'.
    // For .tmj: solid iff the TSX "solid" property is true for that tile type.
    // Out-of-bounds coords always return false.
    bool IsSolid(int tx, int ty) const;

    // Returns the canonical tile char at (tx, ty), or ' ' if out of bounds.
    char GetTile(int tx, int ty) const;

    int GetWidth()  const { return width_; }
    int GetHeight() const { return height_; }

private:
    std::vector<std::string>      rows_;        // char grid ('0','E','K','X',' ')
    std::vector<std::vector<bool>> solid_grid_; // parallel solid-flag grid
    int width_  = 0;
    int height_ = 0;

    bool LoadTxt(const char* path);
    bool LoadTmj(const char* path);
};


// ==========================================================================
// FILE : LevelManager.h
// PATH : src/server/LevelManager.h
// ==========================================================================

#pragma once
// SRP: loads maps, calculates the optimal spawn point, builds canonical level paths.
// No ENet or session-logic dependency.
#include "World.h"
#include <string>
#include <cstdint>

class LevelManager {
public:
    // Load map from path. Returns false if the file cannot be read.
    bool Load(const char* path);

    // Build the canonical path for a level number (e.g. 2 → "assets/levels/tilemaps/Level02.tmj").
    static std::string BuildPath(int num);

    float        SpawnX()   const { return spawn_x_; }
    float        SpawnY()   const { return spawn_y_; }
    const World& GetWorld() const { return world_; }

private:
    World world_;
    float spawn_x_ = 0.f;
    float spawn_y_ = 0.f;
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
//   with_kill = false → respawn_grace_ticks = 60  (triggers Ready/Go! overlay on the client)
// Also clears checkpoint_x/checkpoint_y and resets level_ticks (full restart semantics).
inline PlayerState SpawnReset(PlayerState s, float sx, float sy, bool with_kill) { /* body stripped */ }

// Respawn the player at a checkpoint position.
// Unlike SpawnReset, level_ticks keeps accumulating (time penalty only from downtime)
// and the checkpoint itself is preserved.
// with_kill = true  → kill_respawn_ticks = 60 (used by automatic kill-tile death)
// with_kill = false → respawn_grace_ticks = 60 (used by manual restart-at-checkpoint)
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

// Blocking ENet server loop. Caller must call enet_initialize() beforehand.
// Returns only when stop_flag is set to true.
void RunServer(uint16_t port, const char* map_path, std::atomic<bool>& stop_flag);


// ==========================================================================
// FILE : ServerSession.h
// PATH : src/server/ServerSession.h
// ==========================================================================

#pragma once
// SRP: game session state machine for the server.
// Manages connected players, lobby / game / results phases, and level progression.
// Socket lifecycle and the ENet service loop live in RunServer (ServerLogic.cpp).
#include "LevelManager.h"
#include "Player.h"
#include "Protocol.h"
#include <enet/enet.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstdint>

class ServerSession {
public:
    // Load the initial map. Check IsReady() after construction.
    ServerSession(const char* initial_map_path, int initial_level);

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
    void UpdateZone();
    bool AllInZone()        const;
    uint32_t CountdownTicks() const;
    PlayerState ApplySpawnReset(PlayerState s, bool with_kill) const;

    LevelManager level_mgr_;

    std::string  initial_map_path_;
    int          initial_level_;
    int          current_level_;

    bool         is_ready_           = false;
    bool         in_lobby_            = false;
    bool         game_locked_         = false;
    bool         in_results_          = false;
    bool         in_global_results_   = false;  // true while showing session-end global leaderboard

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
static constexpr Color CLRS_TILE_SPAWN      = {106, 111,  50, 255};  // 'X' spawn point
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
#include <raylib.h>
#include <unordered_map>
#include <string>
#include <cstdint>

class Renderer;

class GameSession {
public:
    struct Config {
        const char* map_path   = nullptr;
        const char* username   = "";
        bool        is_offline = false;  // true → connects to a LocalServer instance
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
    GlobalResultEntry global_results_entries_[MAX_PLAYERS] = {};

    bool     prev_finished_ = false;
    uint32_t best_ticks_    = 0;
    bool     show_record_   = false;

    float   last_safe_x_           = 0.f;
    float   last_safe_y_           = 0.f;
    uint8_t prev_kill_ticks_local_ = 0;

    PauseState pause_state_    = PauseState::PLAYING;
    int        pause_focused_  = 0;   // 0=Resume, 1=Quit to Menu
    int        confirm_focused_= 0;   // 0=No, 1=Yes

    bool        session_over_ = false;
    std::string end_message_;
    std::string end_sub_msg_;
    Color       end_color_{255, 80, 80, 255};

    // Queued disconnect reason received via PKT_VERSION_MISMATCH before the ENet DISCONNECT event.
    std::string pending_disc_reason_;
    std::string pending_disc_sub_;

    void HandlePauseInput(Renderer& renderer);
    void TickFixed(NetworkClient& net);
    void PollNetwork(NetworkClient& net);
    void HandlePacket(const uint8_t* data, size_t size, NetworkClient& net);
    void HandleDisconnect(uint32_t disconnect_data);
    void LoadLevel(const char* path);
    void UpdateLiveBestTicks();
    void BuildLiveLeaderboard(LiveLeaderEntry* out, int& count) const;
    void DoRender(float draw_x, float draw_y, float dt,
                  NetworkClient& net, Renderer& renderer);
};


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
#include <raylib.h>
#include "InputFrame.h"

class InputSampler {
public:
    // Capture all IsKeyPressed / IsGamepadButtonPressed for this render frame.
    void Poll();

    // Sticky flags — return true once then reset to false.
    bool ConsumeJumpPressed()        { bool v = jump_pressed_;           jump_pressed_           = false; return v; }
    bool ConsumeDashPending()        { bool v = dash_pending_;           dash_pending_           = false; return v; }
    bool ConsumePauseToggle()        { bool v = pause_toggle_;           pause_toggle_           = false; return v; }
    // Restart from last checkpoint (or spawn if none): Backspace / Circle
    bool ConsumeRestartRequest()     { bool v = restart_checkpoint_;     restart_checkpoint_     = false; return v; }
    // Restart always from level spawn, clearing checkpoint: Delete / Triangle
    bool ConsumeRestartSpawn()       { bool v = restart_spawn_;          restart_spawn_          = false; return v; }

    // Pause menu navigation — refreshed each render frame; do not consume.
    bool PauseNavUp()   const { return nav_up_;   }
    bool PauseNavDown() const { return nav_down_; }
    bool PauseNavOk()   const { return nav_ok_;   }

    // Live hardware state — safe to call every fixed tick.
    float GetMoveX()                       const;
    void  GetDashDir(float& dx, float& dy) const;
    bool  IsJumpHeld()                     const;

private:
    static constexpr int   GP          = 0;
    static constexpr float GP_DEADZONE = 0.25f;

    bool  jump_pressed_       = false;
    bool  dash_pending_       = false;
    bool  restart_checkpoint_ = false;  // Backspace / Circle
    bool  restart_spawn_      = false;  // Delete   / Triangle
    bool  pause_toggle_       = false;

    bool  nav_up_   = false;
    bool  nav_down_ = false;
    bool  nav_ok_   = false;

    float prev_stick_y_ = 0.f;  // previous-frame stick Y for edge detection in pause navigation
};


// ==========================================================================
// FILE : LocalServer.h
// PATH : src/client/LocalServer.h
// ==========================================================================

#pragma once
#include <cstdint>
#include <atomic>
#include <thread>
#include <string>

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
    void Start(uint16_t port, const char* map_path);

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
// The window must already be open (InitWindow already called).
void ShowSplashScreen(Font& font);

// Blocking menu loop — returns only when the user confirms a choice.
// The window must already be open (InitWindow already called).
// Reads initial field values from `save` and writes them back before returning.
MenuResult ShowMainMenu(Font& font, SaveData& save);


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
    void DrawPlayer(float rx, float ry, const PlayerState& s, bool is_local = true);

    // Screen-space HUD
    void DrawHUD(const PlayerState& s, uint32_t player_count, bool show_players);
    void DrawNetStats(uint32_t rtt, uint32_t jitter, uint32_t loss_pct);
    void DrawTimer(const PlayerState& s,
                   uint32_t best_ticks, uint32_t time_limit_secs,
                   uint32_t next_level_cd_ticks);
    void DrawLiveLeaderboard(const LiveLeaderEntry* entries, int count);
    void DrawNewRecord(bool show, bool is_lobby);
    void DrawLobbyHints(uint32_t cd_ticks, uint32_t player_count);

    // Off-screen player indicators: orange dot + name on the viewport border (~64 px margin).
    // Call after EndWorldDraw, before any other HUD element.
    void DrawOffscreenArrows(const GameState& gs, uint32_t local_player_id);

    // Ready? / Go! animated overlay.
    // Call every frame with the current respawn_grace_ticks value and dt.
    // Phase transitions are managed internally.
    void DrawReadyGo(uint8_t grace_ticks, float dt);
    void ResetReadyGo();   // call on every level or session load

    void DrawDebugPanel(const PlayerState& s);

    // Pause menu
    Rectangle GetPauseItemRect(int item_index) const;  // for mouse hit-testing in GameSession
    void DrawPauseMenu(PauseState state, int focused, int confirm_focused);

    // End-of-level results screen
    void DrawResultsScreen(bool in_results, bool local_ready,
                           const ResultEntry* entries, uint8_t count, uint8_t level,
                           double elapsed_since_start, double total_duration);

    // Session-end global leaderboard (shown after the last level).
    void DrawGlobalResultsScreen(bool in_global, bool local_ready,
                                 const GlobalResultEntry* entries, uint8_t count,
                                 uint8_t total_levels,
                                 double elapsed_since_start, double total_duration);

    // Full-screen error / session-end overlays used in mini-loops inside main.cpp
    void DrawConnectionErrorScreen(const char* msg);
    void DrawSessionEndScreen(const char* main_msg, const char* sub_msg, Color col);

private:
    Font     font_small_ = {};
    Font     font_hud_   = {};
    Font     font_timer_ = {};
    Font     font_bold_  = {};   // loaded at 200 px — all draw sizes are downscales, no blur
    Camera2D camera_     = {};
    float    cam_shake_timer_ = 0.f;

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
};

// Load from "save.dat" in the CWD. Returns true if the file was valid.
// On error (missing file, bad checksum) the struct is left zero-initialised.
bool LoadSaveData(SaveData& out);

void SaveSaveData(const SaveData& data);


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

enum class PauseState { PLAYING, PAUSED, CONFIRM_QUIT };


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

