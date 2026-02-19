#pragma once
#include <SDL3/SDL.h>

class InputManager {
public:
    InputManager();
    ~InputManager();

    void SetGamepad(SDL_Gamepad* gp);
    void Update();

    // Input queries
    void GetRawInput(float* out_x, float* out_y) const;
    bool IsJumpDown() const { return jump_down; }
    bool IsJumpJustPressed() const { return jump_just_pressed; }
    bool IsDashDown() const { return dash_down; }

private:
    SDL_Gamepad* gamepad;
    
    bool jump_down;
    bool prev_jump_down;
    bool jump_just_pressed;
    bool dash_down;

    static const float ANALOG_THRESHOLD;
};
