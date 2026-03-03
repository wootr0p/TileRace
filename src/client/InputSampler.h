#pragma once
// Centralised keyboard + gamepad sampling.
//
// Per-frame usage:
//   1. Call Poll() once per render frame — captures rising-edge events.
//   2. Call ConsumeXxx() to read sticky flags; each returns true exactly once.
//   3. Call GetMoveX() / GetDashDir() / IsJumpHeld() inside each fixed tick
//      (they read live hardware state on every call).
#include <raylib.h>
#include "InputFrame.h"

class InputSampler {
public:
    // Capture all IsKeyPressed / IsGamepadButtonPressed for this render frame.
    void Poll();

    // Sticky flags — return true once then reset to false.
    bool ConsumeJumpPressed()    { bool v = jump_pressed_;    jump_pressed_    = false; return v; }
    bool ConsumeDashPending()    { bool v = dash_pending_;    dash_pending_    = false; return v; }
    bool ConsumePauseToggle()    { bool v = pause_toggle_;    pause_toggle_    = false; return v; }
    bool ConsumeRestartRequest() { bool v = restart_pending_; restart_pending_ = false; return v; }

    // Pause menu navigation — refreshed each render frame; do not consume.
    bool PauseNavUp()   const { return nav_up_;   }
    bool PauseNavDown() const { return nav_down_; }
    bool PauseNavOk()   const { return nav_ok_;   }

    // Live hardware state — safe to call every fixed tick.
    float GetMoveX()                       const;
    void  GetDashDir(float& dx, float& dy) const;
    bool  IsJumpHeld()                     const;

private:
    static constexpr int   GP          = 0;
    static constexpr float GP_DEADZONE = 0.25f;

    bool  jump_pressed_    = false;
    bool  dash_pending_    = false;
    bool  restart_pending_ = false;
    bool  pause_toggle_    = false;

    bool  nav_up_   = false;
    bool  nav_down_ = false;
    bool  nav_ok_   = false;

    float prev_stick_y_ = 0.f;  // previous-frame stick Y for edge detection in pause navigation
};
