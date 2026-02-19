#pragma once
#include <SDL3/SDL.h>

struct TrailGhost {
    float x, y;
    float alpha;
    bool active;
};

class TrailManager {
public:
    TrailManager();
    ~TrailManager();

    void Update(float dt, float player_x, float player_y, bool is_dashing);
    void Render(SDL_Renderer* renderer);

private:
    void EmitGhost(float x, float y);

    TrailGhost* ghosts;
    int max_ghosts;
    float emit_timer;

    static const int MAX_GHOSTS;
    static const float EMIT_RATE;
    static const float FADE_RATE;
};
