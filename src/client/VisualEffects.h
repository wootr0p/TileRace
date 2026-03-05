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

enum class PauseState { PLAYING, PAUSED, CONFIRM_QUIT, LOBBY_SETTINGS };

// Emote bubble — shown above a player after they trigger an emote.
struct EmoteBubble {
    uint8_t emote_id = 0;      // 0-7 index into EMOTE_TEXTS
    float   timer    = 0.f;    // counts down from EMOTE_DURATION to 0
    bool    active   = false;
    float   last_x   = 0.f;    // last known world position (frozen on death)
    float   last_y   = 0.f;
};
