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
    explicit InputSampler(int gamepad_index = 0) : gp_index_(gamepad_index) {}

    // Capture all IsKeyPressed / IsGamepadButtonPressed for this render frame.
    void Poll();

    // Sticky flags — return true once then reset to false.
    bool ConsumeJumpPressed()        { bool v = jump_pressed_;           jump_pressed_           = false; return v; }
    bool ConsumeDashPending()        { bool v = dash_pending_;           dash_pending_           = false; return v; }
    bool ConsumePauseToggle()        { bool v = pause_toggle_;           pause_toggle_           = false; return v; }
    // Restart from last shared checkpoint (or spawn if none): Backspace / Triangle
    bool ConsumeRestartRequest()     { bool v = restart_checkpoint_;     restart_checkpoint_     = false; return v; }
    // Restart always from level spawn, clearing checkpoint: Delete
    bool ConsumeRestartSpawn()       { bool v = restart_spawn_;          restart_spawn_          = false; return v; }

    // Pause menu navigation — refreshed each render frame; do not consume.
    bool PauseNavUp()   const { return nav_up_;   }
    bool PauseNavDown() const { return nav_down_; }
    bool PauseNavOk()   const { return nav_ok_;   }

    // Live hardware state — safe to call every fixed tick.
    float GetMoveX()                       const;
    void  GetDashDir(float& dx, float& dy) const;
    bool  IsJumpHeld()                     const;
    bool  IsDrawHeld()                      const;
    bool  IsSprintHeld()                     const;
    bool  IsMagnetHeld()                     const;

    // Emote wheel (E key / right-stick click).
    bool IsEmoteWheelOpen()       const { return emote_wheel_open_; }
    int  GetEmoteWheelHighlight() const { return emote_highlight_; }  // -1 = none, 0-7 = direction
    int  ConsumeEmotePending()           { int v = emote_pending_; emote_pending_ = -1; return v; }

private:
    int            gp_index_   = 0;
    static constexpr float GP_DEADZONE = 0.25f;

    bool  jump_pressed_       = false;
    bool  dash_pending_       = false;
    bool  restart_checkpoint_ = false;  // Backspace / Triangle
    bool  restart_spawn_      = false;  // Delete
    bool  pause_toggle_       = false;

    bool  nav_up_   = false;
    bool  nav_down_ = false;
    bool  nav_ok_   = false;

    float prev_stick_y_ = 0.f;  // previous-frame stick Y for edge detection in pause navigation

    // Emote wheel state
    bool  emote_wheel_open_ = false;  // true while activation key is held
    int   emote_highlight_  = -1;     // currently highlighted direction (-1 = none)
    int   emote_pending_    = -1;     // emote to send (-1 = none)
    bool  emote_kb_was_open_= false;  // previous-frame E key state (edge detection)
    bool  emote_gp_was_open_= false;  // previous-frame L1 state

    void PollEmote();  // called by Poll()
};
