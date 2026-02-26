#include "InputHandler.h"
#include <raylib.h>

void InputHandler::Poll() {
    // Viene chiamata subito dopo BeginDrawing(), quindi i KeyPressed appena
    // rilevati da Raylib sono ancora validi. Li accumuliamo qui;
    // il flag rimane a true finché Sample() non lo consuma.
    if (IsKeyPressed(KEY_SPACE)) {
        jump_pending_ = true;
    }
    // Dash: Shift (sinistro o destro) — sticky per non perdere pressioni brevi
    if (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT)) {
        dash_pending_ = true;
    }
}

InputFrame InputHandler::Sample(uint32_t current_tick) {
    InputFrame frame{};
    frame.tick    = current_tick;
    frame.buttons = 0;

    // Movimento: IsKeyDown gestisce l'input continuo correttamente
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) frame.buttons |= BTN_LEFT;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) frame.buttons |= BTN_RIGHT;

    // Salto: consuma il flag sticky accumulato da Poll()
    if (jump_pending_) {
        frame.buttons  |= BTN_JUMP;
        jump_pending_   = false;
    }

    // Dash: consuma il flag sticky
    if (dash_pending_) {
        frame.buttons |= BTN_DASH;
        dash_pending_  = false;
    }

    return frame;
}
