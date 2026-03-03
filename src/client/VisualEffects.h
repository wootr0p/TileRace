#pragma once
// Client-only visual effect data structures.
// No Renderer or ENet dependency — only Raylib (Color, Vector2) and Physics.h constants.
#include <raylib.h>
#include <cstdint>

// Dash trail — ring buffer of recent player positions, rendered as fading segments.
struct TrailState {
    struct Point { float x, y; };
    static constexpr int LEN = 12;
    Point pts[LEN] = {};
    int   count    = 0;

    void Push(float x, float y) {
        for (int i = LEN - 1; i > 0; i--) pts[i] = pts[i - 1];
        pts[0] = {x, y};
        if (count < LEN) count++;
    }
    void Clear() { count = 0; }
};

// Death burst particles — spawned the frame kill_respawn_ticks transitions from 0 to > 0.
struct DeathParticle { float x, y, vx, vy, life; };

struct DeathParticles {
    static constexpr int MAX = 18;
    DeathParticle parts[MAX] = {};
    Color         col        = {};

    void Spawn(float cx, float cy, Color c);
    void Update(float dt);
    bool Active() const;
};

// Live in-game leaderboard entry.
struct LiveLeaderEntry {
    uint32_t player_id  = 0;
    uint32_t best_ticks = 0;
    char     name[16]   = {};
};

enum class PauseState { PLAYING, PAUSED, CONFIRM_QUIT };
