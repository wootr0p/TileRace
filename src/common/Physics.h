#pragma once
#include <cstdint>

// =============================================================================
// Physics.h — Costanti e funzioni fisiche pure (stateless)
// =============================================================================
//
// PRINCIPIO: le funzioni fisiche NON modificano nessun stato globale.
// Ricevono dati, restituiscono dati. Nessun side-effect.
//
// Questo garantisce il DETERMINISMO: dati gli stessi input, si ottiene
// sempre lo stesso output, sia sul server che sul client.
//
// Challenge: cosa significa "deterministico" in floating point?
// Hint: cerca "floating point determinism" e "FP_CONTRACT".
// =============================================================================

namespace Physics {

// -------------------------
// Costanti del gioco
// -------------------------
inline constexpr float TILE_SIZE       = 32.0f;
inline constexpr float GRAVITY         = 980.0f;
inline constexpr float MOVE_SPEED      = 200.0f;
inline constexpr float JUMP_FORCE      = -675.0f;   // -900 * 0.75
inline constexpr float MAX_FALL_SPEED  = 1200.0f;

// Wall jump
inline constexpr float WALL_JUMP_VEL_Y = -800.0f;  // componente verticale
inline constexpr float WALL_JUMP_VEL_X = 320.0f;   // componente orizzontale (via dal muro)

// Dash
inline constexpr float   DASH_SPEED           = 650.0f;
inline constexpr uint8_t DASH_DURATION_TICKS  = 8;    // ~133ms
inline constexpr uint8_t DASH_COOLDOWN_TICKS  = 50;   // ~833ms

// -------------------------
// Fixed Timestep
// -------------------------
inline constexpr float  FIXED_DT    = 1.0f / 60.0f;
inline constexpr float  FIXED_DT_MS = FIXED_DT * 1000.0f;

} // namespace Physics
