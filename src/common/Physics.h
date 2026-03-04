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

// Dash push — force multiplier applied to DASH_SPEED when a dashing player hits another
inline constexpr float DASH_PUSH_MULTIPLIER   = 2.0f;  // pushed player receives 2× dash velocity
