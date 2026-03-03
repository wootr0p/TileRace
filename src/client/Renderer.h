#pragma once
// Owns all Raylib draw calls.
// Rule: no file other than Renderer.cpp may call DrawXxx, BeginDrawing, BeginMode2D, etc.
// Replacing Raylib requires rewriting only Renderer.cpp.
#include <raylib.h>
#include <cstdint>
#include "VisualEffects.h"
#include "GameState.h"
#include "Protocol.h"   // EMOTE_TEXTS, EMOTE_COUNT

class  World;
struct PlayerState;
struct ResultEntry;
struct GlobalResultEntry;

class Renderer {
public:
    void Init();
    void Cleanup();
    Font& HudFont();   // exposed for ShowMainMenu (the only external consumer)

    // Camera
    void ResetCamera(float cx, float cy);
    void UpdateCamera(float cx, float cy, float dt);
    void SnapCamera(float cx, float cy);
    void TriggerShake(float duration);
    void TickShake(float dt);

    // Frame lifecycle
    void BeginFrame();
    void EndFrame();
    void BeginWorldDraw();   // enters Camera2D mode
    void EndWorldDraw();     // exits Camera2D mode

    // World-space draw calls — must be between BeginWorldDraw / EndWorldDraw
    void DrawTilemap(const World& world);
    void DrawTrail(const TrailState& t, bool is_local);
    void DrawDeathParticles(const DeathParticles& dp);
    void DrawPlayer(float rx, float ry, const PlayerState& s, bool is_local = true);

    // Emote system (world-space, call between BeginWorldDraw / EndWorldDraw)
    void DrawEmoteWheel(float cx, float cy, int highlighted_dir);  // local-only wheel overlay
    void DrawEmoteBubble(float px, float py, uint8_t emote_id, float alpha, bool is_local);

    // Screen-space HUD
    void DrawHUD(const PlayerState& s, uint32_t player_count, bool show_players);
    void DrawLevelIndicator(uint8_t level);  // bottom-center level number
    void DrawNetStats(uint32_t rtt, uint32_t jitter, uint32_t loss_pct);
    void DrawTimer(const PlayerState& s,
                   uint32_t best_ticks, uint32_t time_limit_secs,
                   uint32_t next_level_cd_ticks);
    void DrawLiveLeaderboard(const LiveLeaderEntry* entries, int count);
    void DrawNewRecord(bool show, bool is_lobby);
    void DrawLobbyHints(uint32_t cd_ticks, uint32_t player_count);

    // Off-screen player indicators: orange dot + name on the viewport border (~64 px margin).
    // Call after EndWorldDraw, before any other HUD element.
    void DrawOffscreenArrows(const GameState& gs, uint32_t local_player_id);

    // Ready? / Go! animated overlay.
    // Call every frame with the current respawn_grace_ticks value and dt.
    // Phase transitions are managed internally.
    void DrawReadyGo(uint8_t grace_ticks, float dt);
    void ResetReadyGo();   // call on every level or session load

    void DrawDebugPanel(const PlayerState& s);

    // Pause menu
    Rectangle GetPauseItemRect(int item_index) const;  // for mouse hit-testing in GameSession
    void DrawPauseMenu(PauseState state, int focused, int confirm_focused);

    // End-of-level results screen
    void DrawResultsScreen(bool in_results, bool local_ready,
                           const ResultEntry* entries, uint8_t count, uint8_t level,
                           double elapsed_since_start, double total_duration);

    // Session-end global leaderboard (shown after the last level).
    void DrawGlobalResultsScreen(bool in_global, bool local_ready,
                                 const GlobalResultEntry* entries, uint8_t count,
                                 uint8_t total_levels,
                                 double elapsed_since_start, double total_duration);

    // Full-screen error / session-end overlays used in mini-loops inside main.cpp
    void DrawConnectionErrorScreen(const char* msg);
    void DrawSessionEndScreen(const char* main_msg, const char* sub_msg, Color col);

private:
    Font     font_small_ = {};
    Font     font_hud_   = {};
    Font     font_timer_ = {};
    Font     font_bold_  = {};   // loaded at 200 px — all draw sizes are downscales, no blur
    Camera2D camera_     = {};
    float    cam_shake_timer_ = 0.f;

    enum class ReadyGoPhase { NONE, READY, GO };
    ReadyGoPhase rg_phase_      = ReadyGoPhase::NONE;
    float        rg_go_timer_   = 0.f;   // seconds elapsed in the GO phase
    uint8_t      rg_prev_grace_ = 0;     // grace_ticks from the previous frame (detects 0→>0 transition)
};
