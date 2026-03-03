// InputSampler.cpp — implementazione del campionamento input.

#include "InputSampler.h"
#include <cmath>

void InputSampler::Poll() {
    const bool gp = IsGamepadAvailable(GP);

    // --- Flag sticky di gioco ---
    if (IsKeyPressed(KEY_SPACE) ||
        (gp && IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)))
        jump_pressed_ = true;

    if (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT) ||
        (gp && (IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_RIGHT_FACE_LEFT) ||
                IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))))
        dash_pending_ = true;

    if (IsKeyPressed(KEY_BACKSPACE) ||
        (gp && IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)))  // Circle
        restart_checkpoint_ = true;

    if (IsKeyPressed(KEY_DELETE) ||
        (gp && IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_RIGHT_FACE_UP)))     // Triangle
        restart_spawn_ = true;

    // --- Toggle pausa ---
    if (IsKeyPressed(KEY_ESCAPE) ||
        (gp && IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_MIDDLE_RIGHT)))
        pause_toggle_ = true;

    // --- Navigazione pause menu ---
    // Edge detection dello stick verticale
    const float gp_sy    = gp ? GetGamepadAxisMovement(GP, GAMEPAD_AXIS_LEFT_Y) : 0.f;
    const bool  stick_up   = (prev_stick_y_ > -0.5f && gp_sy <= -0.5f);
    const bool  stick_down = (prev_stick_y_ <  0.5f && gp_sy >=  0.5f);
    prev_stick_y_ = gp_sy;

    nav_up_ = IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W) ||
              IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A) ||
              (gp && (IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_LEFT_FACE_UP) ||
                      IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_LEFT_FACE_LEFT))) ||
              stick_up;

    nav_down_ = IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S) ||
                IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) ||
                (gp && (IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_LEFT_FACE_DOWN) ||
                        IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))) ||
                stick_down;

    nav_ok_ = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
              (gp && IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
}

// ---------------------------------------------------------------------------
// Stato hardware corrente (senza sticky; richiamabili più volte per tick)
// ---------------------------------------------------------------------------

float InputSampler::GetMoveX() const {
    const bool gp = IsGamepadAvailable(GP);
    float move_x = 0.f;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) move_x -= 1.f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) move_x += 1.f;
    if (gp) {
        const float ax = GetGamepadAxisMovement(GP, GAMEPAD_AXIS_LEFT_X);
        if (fabsf(ax) > GP_DEADZONE) {
            move_x = ax;
        } else if (fabsf(move_x) < 0.01f) {
            if (IsGamepadButtonDown(GP, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) move_x += 1.f;
            if (IsGamepadButtonDown(GP, GAMEPAD_BUTTON_LEFT_FACE_LEFT))  move_x -= 1.f;
        }
    }
    const float clamped = move_x < -1.f ? -1.f : (move_x > 1.f ? 1.f : move_x);
    return clamped;
}

void InputSampler::GetDashDir(float& dx, float& dy) const {
    const bool gp = IsGamepadAvailable(GP);
    dx = 0.f; dy = 0.f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dx += 1.f;
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) dx -= 1.f;
    if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) dy += 1.f;
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) dy -= 1.f;
    if (gp) {
        const float ax = GetGamepadAxisMovement(GP, GAMEPAD_AXIS_LEFT_X);
        const float ay = GetGamepadAxisMovement(GP, GAMEPAD_AXIS_LEFT_Y);
        if (ax * ax + ay * ay > GP_DEADZONE * GP_DEADZONE) {
            dx = ax; dy = ay;
        } else {
            if (IsGamepadButtonDown(GP, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) dx += 1.f;
            if (IsGamepadButtonDown(GP, GAMEPAD_BUTTON_LEFT_FACE_LEFT))  dx -= 1.f;
            if (IsGamepadButtonDown(GP, GAMEPAD_BUTTON_LEFT_FACE_DOWN))  dy += 1.f;
            if (IsGamepadButtonDown(GP, GAMEPAD_BUTTON_LEFT_FACE_UP))    dy -= 1.f;
        }
    }
}

bool InputSampler::IsJumpHeld() const {
    return IsKeyDown(KEY_SPACE) ||
           (IsGamepadAvailable(GP) &&
            IsGamepadButtonDown(GP, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
}
