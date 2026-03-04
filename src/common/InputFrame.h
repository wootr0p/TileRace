#pragma once
#include <cstdint>

// Button bitmask. Rising-edge flags (JUMP_PRESS, DASH) are set in InputSampler::Poll()
// and cleared after one fixed tick so they fire exactly once per physical press.
enum InputBits : uint16_t {
    BTN_LEFT       = 1 << 0,  // move left (held)
    BTN_RIGHT      = 1 << 1,  // move right (held)
    BTN_JUMP       = 1 << 2,  // jump held; used for variable-height cut
    BTN_JUMP_PRESS = 1 << 3,  // rising edge — sets jump buffer
    BTN_DASH       = 1 << 4,  // rising edge — starts dash
    BTN_UP         = 1 << 5,  // held — dash direction / upward steer
    BTN_DOWN       = 1 << 6,  // held — dash direction / downward steer
    BTN_DRAW       = 1 << 7,  // held — player is drawing a trail on the map
    BTN_SPRINT     = 1 << 8,  // held — sprint (faster horizontal movement)
};

// Deterministic input snapshot for one 60 Hz fixed tick.
// Identical layout on client and server: same inputs + same PlayerState = same result.
struct InputFrame {
    uint32_t tick    = 0;     // monotonic fixed-tick counter (60 Hz)
    uint16_t buttons = 0;     // OR of InputBits

    // Raw direction for dash targeting and in-flight steering.
    // Normalised internally in Player::Simulate / RequestDash / SteerDash.
    float dash_dx = 0.f;
    float dash_dy = 0.f;

    // Horizontal movement axis [-1, 1]. Keyboard/DPAD = ±1.0; analogue stick = raw after deadzone.
    float move_x  = 0.f;

    bool Has(uint16_t b) const { return (buttons & b) != 0; }
};
