#include "InputManager.h"

const float InputManager::ANALOG_THRESHOLD = 0.25f;

InputManager::InputManager()
    : gamepad(nullptr),
	jump{ false, false, false },
	dash{ false, false, false }
{

}

InputManager::~InputManager() {
}

void InputManager::SetGamepad(SDL_Gamepad* gp) {
    gamepad = gp;
}

void InputManager::Update() {
    const bool* keys = SDL_GetKeyboardState(NULL);

    // Update jump
    old_jump_down = jump.down;
    jump.down = keys[SDL_SCANCODE_SPACE] || (gamepad && SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH));
    jump.just_pressed = (old_jump_down == false && jump.down == true);
	jump.just_released = (old_jump_down == true && jump.down == false);

    // Update dash
	old_dash_down = dash.down;
    dash.down = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT] || (gamepad && SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST));
	dash.just_pressed = (old_dash_down == false && dash.down == true);
	dash.just_released = (old_dash_down == true && dash.down == false);
}

vector2 InputManager::GetRawInput() {
    float rx = 0.0f, ry = 0.0f;
    const bool* keys = SDL_GetKeyboardState(NULL);

    // Keyboard input
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) rx -= 1.0f;
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) rx += 1.0f;
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) ry -= 1.0f;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) ry += 1.0f;

    // Gamepad input
    if (gamepad) {
        float ax = (float)SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f;
        float ay = (float)SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f;
        if (SDL_sqrtf(ax * ax + ay * ay) > ANALOG_THRESHOLD) {
            rx = ax;
            ry = ay;
        }

        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT)) rx -= 1.0f;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) rx += 1.0f;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP)) ry -= 1.0f;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN)) ry += 1.0f;
    }

    vector2 out;

    // Normalize
    float mag = SDL_sqrtf(rx * rx + ry * ry);
    if (mag > 0.1f) {
        out.x = rx / mag;
        out.y = ry / mag;
    } else {
        out.x = 0.0f;
        out.y = 0.0f;
    }

    return out;
}
