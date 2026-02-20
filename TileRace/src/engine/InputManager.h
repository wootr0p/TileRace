#pragma once
#include <SDL3/SDL.h>

struct button_state {
    bool down;
    bool just_pressed;
    bool just_released;
};

class InputManager {
public:
    InputManager();
    ~InputManager();

    void SetGamepad(SDL_Gamepad* gp);
    void Update();

    // Input queries
    void GetRawInput(float* out_x, float* out_y) const;
    button_state GetJumpButtonState();
    button_state GetDashButtonState();

private:
    SDL_Gamepad* gamepad;
    
    button_state jump;
    button_state dash;

    bool old_jump_down;
    bool old_dash_down;

    static const float ANALOG_THRESHOLD;
};