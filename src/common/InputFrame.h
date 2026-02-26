#pragma once
#include <cstdint>

// =============================================================================
// InputFrame.h — Istantanea dell'input del giocatore per un singolo tick
// =============================================================================
//
// DOMANDA ARCHITETTUALE: perché separare l'input dalla logica di rendering?
//
// Un giocatore premi un tasto — questo è un EVENTO FISICO (Raylib).
// Ma la simulazione deterministica lavora su DATI DISCRETI per tick.
// InputFrame è il "ponte" tra i due mondi: trasforma un evento fisico in
// un dato serializabile e riproducibile.
//
// Nota: InputFrame vive in /common perché SIA il client (prediction)
// CHE il server (authority) devono simulare la stessa logica partendo
// dallo stesso InputFrame. Se cambi questa struct, cambia il determinismo.
// =============================================================================

// Bit flags per i pulsanti — compatti e serializzabili su rete
enum InputButtons : uint8_t {
    BTN_NONE   = 0,
    BTN_LEFT   = 1 << 0,  // 00000001
    BTN_RIGHT  = 1 << 1,  // 00000010
    BTN_JUMP   = 1 << 2,  // 00000100
    BTN_DASH   = 1 << 3,  // 00001000
};

struct InputFrame {
    uint32_t tick;     // A quale tick appartiene questo input? (per la reconciliation)
    uint8_t  buttons;  // Combinazione di InputButtons via OR bit a bit

    [[nodiscard]] bool IsLeft()  const { return (buttons & BTN_LEFT)  != 0; }
    [[nodiscard]] bool IsRight() const { return (buttons & BTN_RIGHT) != 0; }
    [[nodiscard]] bool IsJump()  const { return (buttons & BTN_JUMP)  != 0; }
    [[nodiscard]] bool IsDash()  const { return (buttons & BTN_DASH)  != 0; }
};
