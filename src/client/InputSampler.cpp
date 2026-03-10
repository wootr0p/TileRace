// InputSampler.cpp — implementazione del campionamento input.

#include "InputSampler.h"
#include <cmath>

void InputSampler::Poll() {
    // Fallback auto-claim: normalmente il gamepad viene assegnato già allo splash
    // screen (via SetGamepadIndex). Questo blocco è una rete di sicurezza per il
    // caso in cui l'istanza venga creata senza un indice pre-assegnato (es. se
    // WindowShouldClose() scatta prima che l'utente premi un tasto).
    // IMPORTANTE: gp_index_ viene impostato una sola volta e non viene MAI
    // resettato. Se il gamepad si disconnette e si riconnette allo stesso indice
    // il gioco riprende automaticamente; se viene premuto un altro gamepad durante
    // la disconnessione, viene ignorato — solo l'indice originale è valido.
    if (gp_index_ < 0) {
        for (int i = 0; i < GP_MAX; ++i) {
            if (!IsGamepadAvailable(i)) continue;
            for (int b = 0; b <= GAMEPAD_BUTTON_RIGHT_THUMB; ++b) {
                if (IsGamepadButtonPressed(i, (GamepadButton)b)) {
                    gp_index_ = i;
                    break;
                }
            }
            if (gp_index_ >= 0) break;
        }
    }

    const bool gp = (gp_index_ >= 0) && IsGamepadAvailable(gp_index_);

    // --- Flag sticky di gioco ---
    if (IsKeyPressed(KEY_SPACE) ||
        (gp && IsGamepadButtonPressed(gp_index_, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)))
        jump_pressed_ = true;

    if (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT) ||
        (gp && IsGamepadButtonPressed(gp_index_, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)))
        dash_pending_ = true;

    if (IsKeyPressed(KEY_BACKSPACE) ||
        (gp && IsGamepadButtonPressed(gp_index_, GAMEPAD_BUTTON_RIGHT_FACE_UP)))     // Triangle
        restart_checkpoint_ = true;

    if (IsKeyPressed(KEY_DELETE))
        restart_spawn_ = true;

    // --- Toggle pausa ---
    if (IsKeyPressed(KEY_ESCAPE) ||
        (gp && IsGamepadButtonPressed(gp_index_, GAMEPAD_BUTTON_MIDDLE_RIGHT)))
        pause_toggle_ = true;

    // --- Navigazione pause menu ---
    // Edge detection dello stick verticale
    const float gp_sy    = gp ? GetGamepadAxisMovement(gp_index_, GAMEPAD_AXIS_LEFT_Y) : 0.f;
    const bool  stick_up   = (prev_stick_y_ > -0.5f && gp_sy <= -0.5f);
    const bool  stick_down = (prev_stick_y_ <  0.5f && gp_sy >=  0.5f);
    prev_stick_y_ = gp_sy;

        nav_up_ = IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W) ||
                  (gp && IsGamepadButtonPressed(gp_index_, GAMEPAD_BUTTON_LEFT_FACE_UP)) ||
              stick_up;

        nav_down_ = IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S) ||
                (gp && IsGamepadButtonPressed(gp_index_, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) ||
                stick_down;

    nav_ok_ = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
              (gp && IsGamepadButtonPressed(gp_index_, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));

    // Emote wheel
    PollEmote();
}

// ---------------------------------------------------------------------------
// Stato hardware corrente (senza sticky; richiamabili più volte per tick)
// ---------------------------------------------------------------------------

float InputSampler::GetMoveX() const {
    const bool gp = (gp_index_ >= 0) && IsGamepadAvailable(gp_index_);
    float move_x = 0.f;
    if (IsKeyDown(KEY_A)) move_x -= 1.f;
    if (IsKeyDown(KEY_D)) move_x += 1.f;
    if (gp) {
        const float ax = GetGamepadAxisMovement(gp_index_, GAMEPAD_AXIS_LEFT_X);
        if (fabsf(ax) > GP_DEADZONE) {
            move_x = ax;
        } else if (fabsf(move_x) < 0.01f) {
            if (IsGamepadButtonDown(gp_index_, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) move_x += 1.f;
            if (IsGamepadButtonDown(gp_index_, GAMEPAD_BUTTON_LEFT_FACE_LEFT))  move_x -= 1.f;
        }
    }
    const float clamped = move_x < -1.f ? -1.f : (move_x > 1.f ? 1.f : move_x);
    return clamped;
}

void InputSampler::GetDashDir(float& dx, float& dy) const {
    const bool gp = (gp_index_ >= 0) && IsGamepadAvailable(gp_index_);
    dx = 0.f; dy = 0.f;
    if (IsKeyDown(KEY_D)) dx += 1.f;
    if (IsKeyDown(KEY_A)) dx -= 1.f;
    if (IsKeyDown(KEY_S)) dy += 1.f;
    if (IsKeyDown(KEY_W)) dy -= 1.f;
    if (gp) {
        const float ax = GetGamepadAxisMovement(gp_index_, GAMEPAD_AXIS_LEFT_X);
        const float ay = GetGamepadAxisMovement(gp_index_, GAMEPAD_AXIS_LEFT_Y);
        if (ax * ax + ay * ay > GP_DEADZONE * GP_DEADZONE) {
            dx = ax; dy = ay;
        } else {
            if (IsGamepadButtonDown(gp_index_, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) dx += 1.f;
            if (IsGamepadButtonDown(gp_index_, GAMEPAD_BUTTON_LEFT_FACE_LEFT))  dx -= 1.f;
            if (IsGamepadButtonDown(gp_index_, GAMEPAD_BUTTON_LEFT_FACE_DOWN))  dy += 1.f;
            if (IsGamepadButtonDown(gp_index_, GAMEPAD_BUTTON_LEFT_FACE_UP))    dy -= 1.f;
        }
    }
}

bool InputSampler::IsJumpHeld() const {
    return IsKeyDown(KEY_SPACE) ||
           ((gp_index_ >= 0) && IsGamepadAvailable(gp_index_) &&
            IsGamepadButtonDown(gp_index_, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
}

bool InputSampler::IsDrawHeld() const {
    if (IsKeyDown(KEY_P)) return true;
    if ((gp_index_ >= 0) && IsGamepadAvailable(gp_index_) &&
        IsGamepadButtonDown(gp_index_, GAMEPAD_BUTTON_RIGHT_TRIGGER_1))  // R1
        return true;
    return false;
}

bool InputSampler::IsSprintHeld() const {
    if (IsKeyDown(KEY_RIGHT_CONTROL)) return true;
    if ((gp_index_ >= 0) && IsGamepadAvailable(gp_index_)) {
        const float rt = GetGamepadAxisMovement(gp_index_, GAMEPAD_AXIS_RIGHT_TRIGGER);
        if (rt > 0.3f) return true;
    }
    return false;
}

bool InputSampler::IsMagnetHeld() const {
    if (IsKeyDown(KEY_LEFT_ALT)) return true;
    if ((gp_index_ >= 0) && IsGamepadAvailable(gp_index_)) {
        const float lt = GetGamepadAxisMovement(gp_index_, GAMEPAD_AXIS_LEFT_TRIGGER);
        if (lt > 0.3f) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Emote wheel polling
// ---------------------------------------------------------------------------

// Map a normalised (dx, dy) to one of 8 compass directions (0=Up .. 7=UpLeft), -1 if deadzone.
static int DirToIndex(float dx, float dy, float deadzone) {
    if (dx * dx + dy * dy < deadzone * deadzone) return -1;
    // atan2 gives angle from positive-X axis; we want 0=Up, clockwise.
    // Rotate 90° so Up maps to angle 0.
    float angle = atan2f(dx, -dy);  // Up=(0,-1) → atan2(0, 1)=0
    if (angle < 0.f) angle += 6.2831853f;  // [0, 2π)
    // Each sector is 2π/8 = 0.7854 rad wide, centered on sector boundary.
    int idx = static_cast<int>((angle + 0.3927f) / 0.7854f) % 8;
    return idx;
}

void InputSampler::PollEmote() {
    const bool gp = (gp_index_ >= 0) && IsGamepadAvailable(gp_index_);

    // --- Keyboard: hold E, arrow keys select, release E to fire ---
    const bool kb_open = IsKeyDown(KEY_E);
    if (kb_open) {
        emote_wheel_open_ = true;
        // Live-read arrow direction (highlight follows held keys)
        float dx = 0.f, dy = 0.f;
        if (IsKeyDown(KEY_RIGHT)) dx += 1.f;
        if (IsKeyDown(KEY_LEFT))  dx -= 1.f;
        if (IsKeyDown(KEY_DOWN))  dy += 1.f;
        if (IsKeyDown(KEY_UP))    dy -= 1.f;
        int dir = DirToIndex(dx, dy, 0.3f);
        emote_highlight_ = dir;  // -1 if no arrow held
    }
    if (!kb_open && emote_kb_was_open_) {
        // E released — fire only if a direction is actively highlighted
        if (emote_highlight_ >= 0) {
            emote_pending_ = emote_highlight_;
        }
        emote_wheel_open_ = false;
        emote_highlight_  = -1;
    }
    emote_kb_was_open_ = kb_open;

    // --- Gamepad: hold L1, right stick selects, release L1 to fire ---
    if (gp) {
        const bool gp_open = IsGamepadButtonDown(gp_index_, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
        if (gp_open) {
            emote_wheel_open_ = true;
            const float rx = GetGamepadAxisMovement(gp_index_, GAMEPAD_AXIS_RIGHT_X);
            const float ry = GetGamepadAxisMovement(gp_index_, GAMEPAD_AXIS_RIGHT_Y);
            // Highlight follows stick in real-time; resets to -1 in deadzone
            emote_highlight_ = DirToIndex(rx, ry, 0.4f);
        }
        if (!gp_open && emote_gp_was_open_) {
            // L1 released — fire only if stick was pointing a direction
            if (emote_highlight_ >= 0) {
                emote_pending_ = emote_highlight_;
            }
            emote_wheel_open_ = false;
            emote_highlight_  = -1;
        }
        emote_gp_was_open_ = gp_open;
    }
}
