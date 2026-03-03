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
