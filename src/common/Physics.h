#pragma once

// Costanti compile-time della simulazione.
// Questo è il posto dove bilanciare i valori durante i playtest.
// Zero dipendenze esterne: solo tipi primitivi.

// --- Geometria ---
inline constexpr int   TILE_SIZE   = 32;       // pixel per tile, lato

// --- Movimento diretto (passo 4) ---
inline constexpr float MOVE_SPEED  = 350.f;    // px / secondo (velocità massima orizzontale)
inline constexpr float MOVE_ACCEL  = 8000.f;   // px / s² — accelerazione verso la velocità target
inline constexpr float MOVE_DECEL  = 6000.f;   // px / s² — decelerazione (stop o inversione)

// --- Fisica (passo 5) ---
inline constexpr float GRAVITY        = 1820.f; // px / s²   — da bilanciare ai playtest
inline constexpr float JUMP_FORCE     =  806.f; // impulso verticale verso l'alto
inline constexpr float MAX_FALL_SPEED = 1170.f; // velocità massima in caduta

// --- Fixed timestep (passo 7) ---
inline constexpr float FIXED_DT = 1.f / 60.f;  // secondi per tick

// --- Salto avanzato (passo 6 ext) ---
inline constexpr int   JUMP_BUFFER_TICKS   = 10;    // tick di buffer prima di atterrare (~167ms)
inline constexpr int   COYOTE_TICKS        =  6;    // tick di grazia dopo aver lasciato il bordo (~100ms)
inline constexpr float JUMP_CUT_MULTIPLIER = 0.45f; // moltiplicatore vel_y al rilascio anticipato di SPAZIO
inline constexpr float WALL_JUMP_FORCE_X   = 507.f; // spinta orizzontale del wall jump (px/s)
inline constexpr float WALL_JUMP_FRICTION  =   5.f; // decadimento esponenziale di vel_x (1/s)
inline constexpr int   WALL_PROBE_REACH    = TILE_SIZE / 4; // px extra oltre il bordo del player per rilevare muri adiacenti (8 px = 1/4 tile)

// --- Dash (passo 17) ---
inline constexpr float DASH_SPEED          = 975.f; // px/s durante il dash
inline constexpr int   DASH_ACTIVE_TICKS   =  12;   // durata del dash (~200ms a 60Hz) — da bilanciare
inline constexpr int   DASH_COOLDOWN_TICKS =  25;   // cooldown dopo dash (~417ms a 60Hz) — da bilanciare
