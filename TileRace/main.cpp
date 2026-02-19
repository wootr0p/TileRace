#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "Game.h"

// These functions are the SDL application callbacks. They simply forward to the
// Game instance stored in the appstate pointer. This keeps the SDL-specific
// entry points small while the Game class implements the actual logic.

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    Game* game = new Game();
    *appstate = game;
    return game->Init(argc, argv);
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (!appstate) return SDL_APP_CONTINUE;
    Game* game = reinterpret_cast<Game*>(appstate);
    return game->HandleEvent(event);
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    if (!appstate) return SDL_APP_CONTINUE;
    Game* game = reinterpret_cast<Game*>(appstate);
    return game->Iterate();
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    if (!appstate) { SDL_Quit(); return; }
    Game* game = reinterpret_cast<Game*>(appstate);
    game->Shutdown(result);
    delete game;
    SDL_Quit();
}