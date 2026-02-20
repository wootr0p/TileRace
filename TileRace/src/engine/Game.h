#pragma once
#include <SDL3/SDL.h>
#include "InputManager.h"
#include "../entities/Player.h"

class Game {
public:
    Game();
    ~Game();

    SDL_AppResult Init(int argc, char* argv[]);
    SDL_AppResult HandleEvent(SDL_Event* event);
    SDL_AppResult Iterate();
    void Shutdown(SDL_AppResult result);

    SDL_Renderer* GetRenderer() const { return renderer; }
    SDL_Gamepad* GetGamepad() const { return gamepad; }
    void SetGamepad(SDL_Gamepad* gp) { gamepad = gp; }

private:
    void UpdateFPSCounter(float dt, uint64_t current_ticks);
    void Render();

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Gamepad* gamepad;

    uint64_t last_ticks;

    // FPS telemetry
    static const int FPS_SAMPLES = 60;
    float fps_buffer[FPS_SAMPLES];
    int fps_index;
    char fps_text[32];
    char debug_text[255];
    uint64_t fps_update_timer;

    InputManager* input;
    Player* player;

    // Configuration
    static const int WINDOW_WIDTH = 1280;
    static const int WINDOW_HEIGHT = 720;
};
