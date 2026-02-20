#include "Game.h"
#include "InputManager.h"
#include "../entities/TileMap.h"
#include <iostream>

TileMap* level;

Game::Game()
    : window(nullptr)
    , renderer(nullptr)
    , gamepad(nullptr)
    , last_ticks(0)
    , fps_index(0)
    , fps_update_timer(0)
    , input(nullptr)
    , player(nullptr)
{
    for (int i = 0; i < FPS_SAMPLES; i++) {
        fps_buffer[i] = 0;
    }
    SDL_snprintf(fps_text, sizeof(fps_text), "fps: 0");
    SDL_snprintf(debug_text, sizeof(debug_text), "");
}

Game::~Game() {
    delete input;
	delete level;
    delete player;
}

SDL_AppResult Game::Init(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);

    SDL_CreateWindowAndRenderer("Demo PS", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);

    if (!window || !renderer) {
        std::cerr << "Failed to create window or renderer" << std::endl;
        return SDL_APP_FAILURE;
    }
    last_ticks = SDL_GetTicksNS();

    input = new InputManager();
    level = new TileMap("level_01");
    player = new Player(input, level->GetPlayerSpawn(), level);

    return SDL_APP_CONTINUE;
}

SDL_AppResult Game::HandleEvent(SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_GAMEPAD_ADDED) {
        gamepad = SDL_OpenGamepad(event->gdevice.which);
        if (input) input->SetGamepad(gamepad);
    }

    if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
        gamepad = nullptr;
        if (input) input->SetGamepad(nullptr);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult Game::Iterate() {
    uint64_t current_ticks = SDL_GetTicksNS();
    float dt = SDL_min((float)(current_ticks - last_ticks) / 1e9f, 0.05f);

    // Update FPS counter (before updating last_ticks)
    UpdateFPSCounter(dt, current_ticks);

    last_ticks = current_ticks;

    // Update game
    player->Update(dt);

    // Render
    Render();

    return SDL_APP_CONTINUE;
}

void Game::Shutdown(SDL_AppResult result) {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
}

void Game::UpdateFPSCounter(float dt, uint64_t current_ticks) {
    // Store current frame time
    fps_buffer[fps_index] = dt;
    fps_index = (fps_index + 1) % FPS_SAMPLES;

    // Update display every 500ms
    if (current_ticks > fps_update_timer) {
        float avg_dt = 0;
        for (int i = 0; i < FPS_SAMPLES; i++) {
            avg_dt += fps_buffer[i];
        }
        float avg_fps = (avg_dt > 0) ? (float)FPS_SAMPLES / avg_dt : 0.0f;
        SDL_snprintf(fps_text, sizeof(fps_text), "fps: %.0f", avg_fps);
        fps_update_timer = current_ticks + 500000000;
    }
}

void Game::Render() {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 10, 10, 15, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Render player
    level->Render(renderer);
    player->Render(renderer);

    // Render UI
    SDL_SetRenderScale(renderer, 2.0f, 2.0f);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, 5, 5, fps_text);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);

    #ifdef _DEBUG
    SDL_RenderDebugText(renderer, 5, WINDOW_HEIGHT - 10, "debug info");
    #endif
    SDL_RenderPresent(renderer);
}
