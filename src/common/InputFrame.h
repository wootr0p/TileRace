#pragma once
#include <cstdint>

// ---------------------------------------------------------------------------
// InputFrame — snapshot dell'input per un singolo tick fisso.
// Struttura deterministica: server e client ne condividono la definizione.
// Tutti i bit sono "campionati" lato client e consumati in Player::Simulate.
// ---------------------------------------------------------------------------

// Bit-flag dei pulsanti.
enum InputBits : uint8_t {
    BTN_LEFT       = 1 << 0,  // tenuto premuto --> movimento sinistra
    BTN_RIGHT      = 1 << 1,  // tenuto premuto --> movimento destra
    BTN_JUMP       = 1 << 2,  // tenuto premuto --> usato per variable-jump cut
    BTN_JUMP_PRESS = 1 << 3,  // edge rising (sticky) --> setta jump buffer
    BTN_DASH       = 1 << 4,  // edge rising (sticky) --> avvia dash
    BTN_UP         = 1 << 5,  // tenuto premuto --> direzione dash / steer verso l'alto
    BTN_DOWN       = 1 << 6,  // tenuto premuto --> direzione dash / steer verso il basso
};

struct InputFrame {
    uint32_t tick    = 0;     // numero del tick fisso (60Hz, monotono)
    uint8_t  buttons = 0;     // OR di InputBits

    // Direzione grezza per dash e sterzata (analogica se gamepad, discreta ±1 se tastiera).
    // Viene normalizzata internamente in Player::Simulate / RequestDash / SteerDash.
    float dash_dx = 0.f;
    float dash_dy = 0.f;

    bool Has(InputBits b) const { return (buttons & b) != 0; }
};
