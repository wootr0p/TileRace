#include "TrailManager.h"

const int TrailManager::MAX_GHOSTS = 30;
const float TrailManager::EMIT_RATE = 0.012f;
const float TrailManager::FADE_RATE = 4.2f;

TrailManager::TrailManager()
    : max_ghosts(MAX_GHOSTS)
    , emit_timer(0.0f)
{
    ghosts = new TrailGhost[max_ghosts];
    for (int i = 0; i < max_ghosts; i++) {
        ghosts[i].active = false;
        ghosts[i].alpha = 0.0f;
        ghosts[i].x = 0.0f;
        ghosts[i].y = 0.0f;
    }
}

TrailManager::~TrailManager() {
    delete[] ghosts;
}

void TrailManager::Update(float dt, float player_x, float player_y, bool is_dashing) {
    // Update ghost alpha
    for (int i = 0; i < max_ghosts; i++) {
        if (ghosts[i].active) {
            ghosts[i].alpha -= FADE_RATE * dt;
            if (ghosts[i].alpha <= 0) {
                ghosts[i].active = false;
            }
        }
    }

    // Emit new ghosts during dash
    emit_timer -= dt;
    if (is_dashing && emit_timer <= 0) {
        EmitGhost(player_x, player_y);
        emit_timer = EMIT_RATE;
    }
}

void TrailManager::EmitGhost(float x, float y) {
    for (int i = 0; i < max_ghosts; i++) {
        if (!ghosts[i].active) {
            ghosts[i].x = x;
            ghosts[i].y = y;
            ghosts[i].alpha = 0.6f;
            ghosts[i].active = true;
            break;
        }
    }
}

void TrailManager::Render(SDL_Renderer* renderer) {
    const int PLAYER_WIDTH = 32;
    const int PLAYER_HEIGHT = 32;

    for (int i = 0; i < max_ghosts; i++) {
        if (ghosts[i].active) {
            SDL_FRect r = { ghosts[i].x, ghosts[i].y, (float)PLAYER_WIDTH, (float)PLAYER_HEIGHT };
            SDL_SetRenderDrawColor(renderer, 255, 120, 0, (Uint8)(ghosts[i].alpha * 180));
            SDL_RenderFillRect(renderer, &r);
        }
    }
}
