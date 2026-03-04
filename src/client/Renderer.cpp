// Renderer.cpp — unico file che chiama le API di disegno di Raylib.
// Se si sostituisce Raylib, si riscrive solo questo file.

#include "Renderer.h"
#include "Colors.h"
#include "World.h"
#include "Physics.h"
#include "Protocol.h"   // ResultEntry, MAX_PLAYERS
#include "PlayerState.h"
#include <cmath>
#include <algorithm>
#include <cstring>

// ---------------------------------------------------------------------------
// Ciclo di vita
// ---------------------------------------------------------------------------
void Renderer::Init() {
    font_small_ = LoadFontEx("assets/fonts/SpaceMono-Regular.ttf",  18, nullptr, 0);
    font_hud_   = LoadFontEx("assets/fonts/SpaceMono-Regular.ttf",  24, nullptr, 0);
    font_timer_ = LoadFontEx("assets/fonts/SpaceMono-Regular.ttf",  48, nullptr, 0);
    // 200 px: copre sia "Ready?" (144) sia "Go!" animato (144×1.35 ≈ 195) senza upscaling.
    font_bold_  = LoadFontEx("assets/fonts/SpaceMono-Bold.ttf",    200, nullptr, 0);
    SetTextureFilter(font_timer_.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(font_bold_.texture,  TEXTURE_FILTER_BILINEAR);

    camera_.rotation = 0.f;
    camera_.zoom     = 1.f;
}

void Renderer::Cleanup() {
    UnloadFont(font_small_);
    UnloadFont(font_hud_);
    UnloadFont(font_timer_);
    UnloadFont(font_bold_);
}

Font& Renderer::HudFont() { return font_hud_; }

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------
static constexpr float CAM_SMOOTH = 8.f;

void Renderer::ResetCamera(float cx, float cy) {
    camera_.offset = { GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };
    camera_.target = { cx, cy };
}

void Renderer::UpdateCamera(float cx, float cy, float dt) {
    camera_.offset = { GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };
    const float k  = 1.f - expf(-CAM_SMOOTH * dt);
    camera_.target.x += (cx - camera_.target.x) * k;
    camera_.target.y += (cy - camera_.target.y) * k;
}

void Renderer::SnapCamera(float cx, float cy) {
    camera_.target = { cx, cy };
}

void Renderer::TriggerShake(float duration) {
    cam_shake_timer_ = duration;
}

void Renderer::TickShake(float dt) {
    if (cam_shake_timer_ <= 0.f) return;
    cam_shake_timer_ -= dt;
    if (cam_shake_timer_ < 0.f) cam_shake_timer_ = 0.f;
    // Rimappa l'offset della camera anche qui: sovrascrive quello impostato da
    // UpdateCamera/ResetCamera questa stessa frame, aggiungendo il rumore.
    camera_.offset = { GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };
    const float mag = (cam_shake_timer_ / 0.4f) * 9.f;
    camera_.offset.x += static_cast<float>(GetRandomValue(-100, 100)) * 0.01f * mag;
    camera_.offset.y += static_cast<float>(GetRandomValue(-100, 100)) * 0.01f * mag;
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------
void Renderer::SetPalette(const LevelPalette& p) {
    palette_ = p;
}

void Renderer::BeginFrame() {
    BeginDrawing();
    ClearBackground(palette_.bg);
}

void Renderer::EndFrame() {
    EndDrawing();
}

void Renderer::BeginWorldDraw() {
    BeginMode2D(camera_);
}

void Renderer::EndWorldDraw() {
    EndMode2D();
}

// ---------------------------------------------------------------------------
// Off-screen player indicators
// ---------------------------------------------------------------------------
void Renderer::DrawOffscreenArrows(const GameState& gs, uint32_t local_player_id) {
    const float sw     = static_cast<float>(GetScreenWidth());
    const float sh     = static_cast<float>(GetScreenHeight());
    constexpr float MARGIN    = 75.f;
    const float left          = MARGIN;
    const float right         = sw - MARGIN;
    const float top           = MARGIN;
    const float bottom        = sh - MARGIN;
    const float cx            = sw * 0.5f;
    const float cy            = sh * 0.5f;
    constexpr float ICON_SIZE = TILE_SIZE * 0.5f;   // 16 px — half a normal player tile
    constexpr float NAME_SZ   = 24.f;               // same size used by DrawPlayer for remote names

    for (uint32_t i = 0; i < gs.count; i++) {
        const PlayerState& rp = gs.players[i];
        if (rp.player_id == 0 || rp.player_id == local_player_id) continue;
        if (rp.kill_respawn_ticks > 0) continue;  // skip during death/respawn animation

        // World-space centre of the player tile
        const float wx = rp.x + TILE_SIZE * 0.5f;
        const float wy = rp.y + TILE_SIZE * 0.5f;

        // Convert to screen space using the current camera
        const Vector2 sp = GetWorldToScreen2D({wx, wy}, camera_);

        // Hide the indicator a bit before the player enters the viewport so the
        // world-space name (drawn above the player rect) doesn't overlap the indicator.
        // HIDE_PAD accounts for the name text width (~15 chars at 24 px ≈ 210 px half).
        constexpr float HIDE_PAD = 120.f;
        if (sp.x >= -HIDE_PAD && sp.x <= sw + HIDE_PAD &&
            sp.y >= -HIDE_PAD && sp.y <= sh + HIDE_PAD) continue;

        // Direction from screen centre to the off-screen player
        const float dx = sp.x - cx;
        const float dy = sp.y - cy;
        if (dx == 0.f && dy == 0.f) continue;

        // Intersect with the margin-inset rectangle to find the indicator position.
        // Track horizontal/vertical t separately to know which edge we landed on.
        float th = 1e18f, tv = 1e18f;
        if (dx > 0.f) th = (right  - cx) / dx;
        if (dx < 0.f) th = (left   - cx) / dx;
        if (dy > 0.f) tv = (bottom - cy) / dy;
        if (dy < 0.f) tv = (top    - cy) / dy;
        const float t  = std::min(th, tv);

        const float ax = cx + dx * t;
        const float ay = cy + dy * t;

        // Color reflects dash availability, same logic as DrawPlayer — 50% alpha
        const bool  dash_ready = rp.dash_active_ticks > 0 || rp.dash_ready;
        const Color base_col   = dash_ready ? CLRS_PLAYER_REMOTE : CLRS_PLAYER_REMOTE_DIM;
        const Color col        = {base_col.r, base_col.g, base_col.b, 128};

        // Square indicator — half a player tile, centred on (ax, ay), no border
        DrawRectangle(
            static_cast<int>(ax - ICON_SIZE * 0.5f),
            static_cast<int>(ay - ICON_SIZE * 0.5f),
            static_cast<int>(ICON_SIZE),
            static_cast<int>(ICON_SIZE),
            col);

        // Name always towards the inside of the screen:
        //   horizontal edge (left/right) → name on the opposite side of the square
        //   vertical edge (top/bottom)   → name on the opposite side of the square
        if (rp.name[0] != '\0') {
            const Vector2 ts  = MeasureTextEx(font_hud_, rp.name, NAME_SZ, 1);
            constexpr float PAD = 10.f;
            float nx, ny;
            if (th <= tv) {
                // Landed on left or right edge — name goes inward (horizontally)
                if (dx > 0.f) {
                    // right edge → name to the LEFT of the square
                    nx = ax - ICON_SIZE * 0.5f - ts.x - PAD;
                } else {
                    // left edge  → name to the RIGHT of the square
                    nx = ax + ICON_SIZE * 0.5f + PAD;
                }
                ny = ay - ts.y * 0.5f;
            } else {
                // Landed on top or bottom edge — name goes inward (vertically)
                nx = ax - ts.x * 0.5f;
                if (dy < 0.f) {
                    // top edge    → name BELOW the square
                    ny = ay + ICON_SIZE * 0.5f + PAD;
                } else {
                    // bottom edge → name ABOVE the square
                    ny = ay - ICON_SIZE * 0.5f - ts.y - PAD;
                }
            }
            constexpr Color NAME_COL = {CLRS_PLAYER_REMOTE_NAME.r, CLRS_PLAYER_REMOTE_NAME.g, CLRS_PLAYER_REMOTE_NAME.b, 128};
            DrawTextEx(font_hud_, rp.name, {nx, ny}, NAME_SZ, 1, NAME_COL);
        }
    }
}

// ---------------------------------------------------------------------------
// World-space
// ---------------------------------------------------------------------------
void Renderer::DrawTilemap(const World& world) {
    for (int ty = 0; ty < world.GetHeight(); ty++) {
        for (int tx = 0; tx < world.GetWidth(); tx++) {
            const char c = world.GetTile(tx, ty);
            Color col;
            if      (c == '0') col = palette_.wall;
            else if (c == 'E') col = palette_.exit_tile;
            else if (c == 'K') col = palette_.kill_tile;
            else if (c == 'X') col = palette_.spawn;
            else if (c == 'C') col = palette_.checkpoint;
            else continue;
            DrawRectangle(tx * TILE_SIZE, ty * TILE_SIZE, TILE_SIZE, TILE_SIZE, col);
        }
    }
}

void Renderer::DrawTrail(const TrailState& t, bool is_local) {
    const Color base = is_local ? CLRS_PLAYER_LOCAL : CLRS_PLAYER_REMOTE;
    for (int i = t.count - 1; i >= 0; i--) {
        const float   frac  = 1.f - static_cast<float>(i + 1) / static_cast<float>(TrailState::LEN + 1);
        const uint8_t alpha = static_cast<uint8_t>(frac * 80.f);
        const int     sh    = i / 2;
        DrawRectangle(
            static_cast<int>(t.pts[i].x) + sh / 2,
            static_cast<int>(t.pts[i].y) + sh / 2,
            TILE_SIZE - sh, TILE_SIZE - sh,
            {base.r, base.g, base.b, alpha});
    }
}

void Renderer::DrawDeathParticles(const DeathParticles& dp) {
    for (const auto& p : dp.parts) {
        if (p.life <= 0.f) continue;
        const uint8_t a  = static_cast<uint8_t>(p.life * 255.f);
        const int     sz = static_cast<int>(p.life * 10.f) + 2;
        DrawRectangle(static_cast<int>(p.x) - sz / 2,
                      static_cast<int>(p.y) - sz / 2,
                      sz, sz, {dp.col.r, dp.col.g, dp.col.b, a});
    }
}

void Renderer::DrawPlayer(float rx, float ry, const PlayerState& s, bool is_local) {
    Color col;
    if (is_local) {
        const bool bright = s.dash_active_ticks > 0 || s.dash_ready;
        col = bright ? CLRS_PLAYER_LOCAL : CLRS_PLAYER_LOCAL_DIM;
    } else {
        const bool bright = s.dash_active_ticks > 0 || s.dash_ready;
        col = bright ? CLRS_PLAYER_REMOTE : CLRS_PLAYER_REMOTE_DIM;
    }
    DrawRectangle(static_cast<int>(rx), static_cast<int>(ry), TILE_SIZE, TILE_SIZE, col);

    // Nome sopra il rettangolo — per tutti i player
    if (s.name[0] != '\0') {
        const float   nm_sz = 24.f;
        const Color   nm_col = is_local
            ? Color{CLRS_PLAYER_LOCAL.r, CLRS_PLAYER_LOCAL.g, CLRS_PLAYER_LOCAL.b, 180}
            : CLRS_PLAYER_REMOTE_NAME;
        const Vector2 nm_ts = MeasureTextEx(font_hud_, s.name, nm_sz, 1);
        DrawTextEx(font_hud_, s.name,
            {rx + TILE_SIZE * 0.5f - nm_ts.x * 0.5f, ry - nm_sz - 4},
            nm_sz, 1, nm_col);
    }
}

// ---------------------------------------------------------------------------
// Emote wheel — world-space, local player only
// ---------------------------------------------------------------------------
void Renderer::DrawEmoteWheel(float cx, float cy, int highlighted_dir) {
    // cx, cy = screen-space center of the wheel
    constexpr float WHEEL_RADIUS  = 120.f;
    constexpr float SLOT_RADIUS   = 32.f;
    constexpr float EMOTE_FONT_SZ = 28.f;
    constexpr float PI2            = 6.2831853f;

    // Semi-transparent background circle
    DrawCircle(static_cast<int>(cx), static_cast<int>(cy),
               WHEEL_RADIUS + SLOT_RADIUS, {0, 0, 0, 120});

    for (int i = 0; i < EMOTE_COUNT; ++i) {
        // Angle: 0=Up, clockwise. atan2 convention: angle from +Y going CW.
        const float angle = (static_cast<float>(i) / EMOTE_COUNT) * PI2 - PI2 * 0.25f;
        const float sx = cx + cosf(angle) * WHEEL_RADIUS;
        const float sy = cy + sinf(angle) * WHEEL_RADIUS;

        const bool hl = (i == highlighted_dir);
        const Color bg_col  = hl ? Color{CLRS_PLAYER_LOCAL.r, CLRS_PLAYER_LOCAL.g, CLRS_PLAYER_LOCAL.b, 220}
                                 : Color{40, 40, 60, 180};
        const Color txt_col = hl ? WHITE : Color{200, 200, 200, 200};

        DrawCircle(static_cast<int>(sx), static_cast<int>(sy), SLOT_RADIUS, bg_col);
        const Vector2 ts = MeasureTextEx(font_hud_, EMOTE_TEXTS[i], EMOTE_FONT_SZ, 1);
        DrawTextEx(font_hud_, EMOTE_TEXTS[i],
                   {sx - ts.x * 0.5f, sy - ts.y * 0.5f},
                   EMOTE_FONT_SZ, 1, txt_col);
    }
}

// ---------------------------------------------------------------------------
// Emote bubble — world-space, shown above any player
// ---------------------------------------------------------------------------
void Renderer::DrawEmoteBubble(float px, float py, uint8_t emote_id, float alpha, bool is_local) {
    if (emote_id >= EMOTE_COUNT || alpha <= 0.f) return;

    const uint8_t a = static_cast<uint8_t>(alpha * 255.f);
    const Color base = is_local ? CLRS_PLAYER_LOCAL : CLRS_PLAYER_REMOTE;
    const Color col  = {base.r, base.g, base.b, a};

    constexpr float FONT_SZ = 40.f;  // large, crisp (font_timer_ loaded at 48)
    const float bx = px + TILE_SIZE * 0.5f;
    const float by = py - 36.f;   // above the player (and name tag)

    const Vector2 ts = MeasureTextEx(font_timer_, EMOTE_TEXTS[emote_id], FONT_SZ, 1);
    DrawTextEx(font_timer_, EMOTE_TEXTS[emote_id],
               {bx - ts.x * 0.5f, by - ts.y * 0.5f},
               FONT_SZ, 1, col);
}

// ---------------------------------------------------------------------------
// Screen-space HUD
// ---------------------------------------------------------------------------
void Renderer::DrawHUD(const PlayerState& s, uint32_t player_count, bool show_players) {
    (void)s;
    const char* txt = show_players
        ? TextFormat("fps: %d  players: %u", GetFPS(), player_count)
        : TextFormat("fps: %d", GetFPS());
    DrawTextEx(font_hud_, txt, {10, 10}, 24, 1, WHITE);
}

void Renderer::DrawLevelIndicator(uint8_t level) {
    if (level == 0) return;
    const char* txt = TextFormat("Level %d", level);
    const float sz  = 24.f;
    const Vector2 ts = MeasureTextEx(font_hud_, txt, sz, 1);
    const float sw = static_cast<float>(GetScreenWidth());
    const float sh = static_cast<float>(GetScreenHeight());
    DrawTextEx(font_hud_, txt, {sw * 0.5f - ts.x * 0.5f, sh - ts.y - 10.f}, sz, 1, WHITE);
}

void Renderer::DrawNetStats(uint32_t rtt, uint32_t jitter, uint32_t loss_pct) {
    Color rtt_col  = rtt  <  60 ? CLRS_STAT_GOOD
                   : rtt  < 120 ? CLRS_STAT_OK
                                : CLRS_STAT_BAD;
    Color loss_col = loss_pct == 0 ? CLRS_STAT_GOOD
                   : loss_pct <  5 ? CLRS_STAT_OK
                                   : CLRS_STAT_BAD;
    const float sz   = 24.f;
    const float sw   = static_cast<float>(GetScreenWidth());
    const float sh   = static_cast<float>(GetScreenHeight());
    const float pad  = 10.f;
    const float line = sz + 2.f;
    const char* rtt_str  = TextFormat("ping: %u ms",   rtt);
    const char* jit_str  = TextFormat("jitter: %u ms", jitter);
    const char* loss_str = TextFormat("loss: %u%%",    loss_pct);
    const Vector2 rs = MeasureTextEx(font_hud_, rtt_str,  sz, 1);
    const Vector2 js = MeasureTextEx(font_hud_, jit_str,  sz, 1);
    const Vector2 ls = MeasureTextEx(font_hud_, loss_str, sz, 1);
    DrawTextEx(font_hud_, loss_str, {sw - ls.x - pad, sh - pad - line * 3.f}, sz, 1, loss_col);
    DrawTextEx(font_hud_, jit_str,  {sw - js.x - pad, sh - pad - line * 4.f}, sz, 1, CLRS_STAT_NEUTRAL);
    DrawTextEx(font_hud_, rtt_str,  {sw - rs.x - pad, sh - pad - line * 5.f}, sz, 1, rtt_col);
}

void Renderer::DrawTimer(const PlayerState& s,
                         uint32_t best_ticks, uint32_t time_limit_secs,
                         uint32_t next_level_cd_ticks) {
    // Countdown is the main center timer.
    const uint32_t mins = time_limit_secs / 60;
    const uint32_t secs_r = time_limit_secs % 60;
    const char* t_str = TextFormat("%02u:%02u", mins, secs_r);
    const Color t_col = (time_limit_secs <= 30 && time_limit_secs > 0)
        ? CLRS_TIME_LIMIT_WARN : WHITE;
    const Vector2 t_sz = MeasureTextEx(font_timer_, t_str, 48, 1);
    DrawTextEx(font_timer_, t_str,
        {GetScreenWidth() * 0.5f - t_sz.x * 0.5f, 10}, 48, 1, t_col);

    // "Next level in: X.XX s" (bottom right)
    if (next_level_cd_ticks > 0) {
        const uint32_t rem_cs = next_level_cd_ticks * 100 / 60;
        const char* cd_str = TextFormat("Next level in: %u.%02u s", rem_cs / 100, rem_cs % 100);
        const Vector2 cd_sz = MeasureTextEx(font_hud_, cd_str, 24, 1);
        DrawTextEx(font_hud_, cd_str,
            {GetScreenWidth()  - cd_sz.x - 10,
             GetScreenHeight() - cd_sz.y - 10},
            24, 1, CLRS_TIMER_BEST);
    }
}

void Renderer::DrawLiveLeaderboard(const LiveLeaderEntry* entries, int count) {
    const float lsz = 24.f;
    const float ly0 = 10.f + lsz + 4.f;
    for (int i = 0; i < count; i++) {
        const LiveLeaderEntry& e = entries[i];
        const uint32_t cs = e.best_ticks * 100 / 60;
        const char* t_str = TextFormat("%s  %02u:%02u.%02u",
            e.name[0] ? e.name : "?",
            cs / 6000, (cs % 6000) / 100, cs % 100);
        DrawTextEx(font_hud_, t_str, {10.f, ly0 + i * (lsz + 2.f)},
            lsz, 1, CLRS_TEXT_HALF_WHITE);
    }
}

void Renderer::DrawNewRecord(bool show, bool is_lobby) {
    if (!show || is_lobby) return;
    const char* msg = "New Record!";
    const Vector2 ms = MeasureTextEx(font_hud_, msg, 24, 1);
    DrawRectangle(
        static_cast<int>(GetScreenWidth()  * 0.5f - ms.x * 0.5f - 12),
        static_cast<int>(GetScreenHeight() * 0.5f - 20),
        static_cast<int>(ms.x + 24), 44,
        CLRS_BG_DARK_SEMI);
    DrawTextEx(font_hud_, msg,
        {GetScreenWidth() * 0.5f - ms.x * 0.5f,
         GetScreenHeight() * 0.5f - 12.f},
        24, 1, CLRS_ACCENT);
}

void Renderer::DrawLobbyHints(uint32_t cd_ticks, uint32_t player_count) {
    const float cx = GetScreenWidth()  * 0.5f;
    const float cy = GetScreenHeight() * 0.5f;
    if (cd_ticks > 0) {
        const uint32_t rem_secs = (cd_ticks + 59u) / 60u;
        const char* cd_str = TextFormat("Game starts in %u...", rem_secs);
        const Vector2 cd_sz = MeasureTextEx(font_timer_, cd_str, 48, 1);
        DrawRectangle(0, static_cast<int>(cy - 40), GetScreenWidth(), 80, {0, 0, 0, 160});
        DrawTextEx(font_timer_, cd_str,
            {cx - cd_sz.x * 0.5f, cy - 24.f}, 48, 1, CLRS_ACCENT);
    } else {
        const char* hint = player_count == 0
            ? "Waiting for players..."
            : "Move ALL players to the EXIT to start!";
        const Vector2 hs = MeasureTextEx(font_hud_, hint, 24, 1);
        DrawTextEx(font_hud_, hint,
            {cx - hs.x * 0.5f, GetScreenHeight() - hs.y - 20.f},
            24, 1, CLRS_LOBBY_HINT);
    }
}

// ---------------------------------------------------------------------------
// Overlay "Ready?" / "Go!" — sostituisce il vecchio conto alla rovescia 3-2-1
// ---------------------------------------------------------------------------
void Renderer::DrawReadyGo(uint8_t grace_ticks, float dt) {
    // --- Transizioni di fase ---
    // Detect: grace_ticks appena impostato (0 → >0): inizia fase READY
    if (rg_prev_grace_ == 0 && grace_ticks > 0) {
        rg_phase_    = ReadyGoPhase::READY;
        rg_go_timer_ = 0.f;
    }
    // Detect: grace_ticks appena azzerato (>0 → 0): inizia fase GO
    if (rg_prev_grace_ > 0 && grace_ticks == 0) {
        rg_phase_    = ReadyGoPhase::GO;
        rg_go_timer_ = 0.f;
    }
    rg_prev_grace_ = grace_ticks;

    // Avanza il timer della fase GO
    if (rg_phase_ == ReadyGoPhase::GO) {
        rg_go_timer_ += dt;
        if (rg_go_timer_ >= 0.50f)
            rg_phase_ = ReadyGoPhase::NONE;
    }

    if (rg_phase_ == ReadyGoPhase::NONE) return;

    const float cx = GetScreenWidth()  * 0.5f;
    const float cy = GetScreenHeight() * 0.5f;

    if (rg_phase_ == ReadyGoPhase::READY) {
        // grace_ticks scende da 25 (= ~0.4s a 60Hz) a 1.
        // t = 1.0 all'inizio, ~0.04 all'ultimo tick.
        // Fade-in:  t 1.0→0.7  → alpha 0→1  (primo 30%)
        // Hold:     t 0.7→0.2  → alpha 1
        // Fade-out: t 0.2→0.0  → alpha 1→0  (ultimo 20%)
        const float t = static_cast<float>(grace_ticks) / 25.f;
        float alpha;
        if      (t > 0.7f) alpha = (1.0f - t) / 0.3f;
        else if (t < 0.2f) alpha = t / 0.2f;
        else               alpha = 1.f;
        alpha = std::max(0.f, std::min(1.f, alpha));

        const uint8_t a   = static_cast<uint8_t>(alpha * 255.f);
        const Color   col = {CLRS_READYGO_READY.r, CLRS_READYGO_READY.g, CLRS_READYGO_READY.b, a};
        const float   sz  = 144.f;
        const float   rot = 8.f;                // leggermente inclinato a destra

        const Vector2 tsz = MeasureTextEx(font_bold_, "Ready?", sz, 1);
        DrawTextPro(font_bold_, "Ready?",
            {cx, cy},
            {tsz.x * 0.5f, tsz.y * 0.5f},
            rot, sz, 1.f, col);

    } else {  // ReadyGoPhase::GO
        // 0→0.2s: piena opacità, dimensione normale
        // 0.2→0.5s: fade out + leggero scale-up (effetto esplosione)
        float alpha, scale;
        if (rg_go_timer_ < 0.20f) {
            alpha = 1.f;
            scale = 1.f;
        } else {
            const float p = (rg_go_timer_ - 0.20f) / 0.30f;  // 0→1
            alpha = 1.f - p;
            scale = 1.f + p * 0.35f;   // fino a 1.35×
        }
        alpha = std::max(0.f, std::min(1.f, alpha));

        const uint8_t a   = static_cast<uint8_t>(alpha * 255.f);
        const Color   col = {CLRS_READYGO_GO.r, CLRS_READYGO_GO.g, CLRS_READYGO_GO.b, a};
        const float   sz  = 144.f * scale;
        const float   rot = -8.f;               // inclinato a sinistra

        const Vector2 tsz = MeasureTextEx(font_bold_, "Go!", sz, 1);
        DrawTextPro(font_bold_, "Go!",
            {cx, cy},
            {tsz.x * 0.5f, tsz.y * 0.5f},
            rot, sz, 1.f, col);
    }
}

void Renderer::ResetReadyGo() {
    rg_phase_      = ReadyGoPhase::NONE;
    rg_go_timer_   = 0.f;
    rg_prev_grace_ = 0;
}

// ---------------------------------------------------------------------------
// Debug
// ---------------------------------------------------------------------------
void Renderer::DrawDebugPanel(const PlayerState& s) {
    const float y0 = static_cast<float>(GetScreenHeight() - 18 * 12 - 50);
    DrawTextEx(font_small_,
        TextFormat(
            "x:             %.0f\n"
            "y:             %.0f\n"
            "on_ground:     %s\n"
            "on_wall_left:  %s\n"
            "on_wall_right: %s\n"
            "last_wj_dir:   %d\n"
            "jump_buf:      %d\n"
            "coyote:        %d\n"
            "vel_x:         %.0f\n"
            "vel_y:         %.0f\n"
            "dash_active:   %d\n"
            "dash_cooldown: %d",
            s.x, s.y,
            s.on_ground     ? "YES" : "no",
            s.on_wall_left  ? "YES" : "no",
            s.on_wall_right ? "YES" : "no",
            static_cast<int>(s.last_wall_jump_dir),
            static_cast<int>(s.jump_buffer_ticks),
            static_cast<int>(s.coyote_ticks),
            s.vel_x, s.vel_y,
            static_cast<int>(s.dash_active_ticks),
            static_cast<int>(s.dash_cooldown_ticks)),
        {10, y0}, 18, 1, CLRS_DEBUG_TEXT);
}

// ---------------------------------------------------------------------------
// Menu di pausa
// ---------------------------------------------------------------------------
Rectangle Renderer::GetPauseItemRect(int idx, int total_items) const {
    const float pcx    = GetScreenWidth()  * 0.5f;
    const float pcy    = GetScreenHeight() * 0.5f;
    const float item_y = pcy - (total_items - 1) * 22.f + idx * 44.f;
    const Vector2 sz   = MeasureTextEx(font_hud_, "  Quit to Menu", 24, 1);
    return Rectangle{pcx - sz.x * 0.5f - 8, item_y - 4, sz.x + 16, 24.f + 8};
}

void Renderer::DrawPauseMenu(PauseState state, int focused, int confirm_focused, bool sfx_muted) {
    if (state == PauseState::PLAYING) return;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), CLRS_BG_OVERLAY);
    const float pcx = GetScreenWidth()  * 0.5f;
    const float pcy = GetScreenHeight() * 0.5f;

    if (state == PauseState::PAUSED) {
        const char* title = "PAUSED";
        const Vector2 ts = MeasureTextEx(font_timer_, title, 48, 1);
        DrawTextEx(font_timer_, title, {pcx - ts.x * 0.5f, pcy - 150.f}, 48, 1, CLRS_ACCENT);

        const char* sfx_label = sfx_muted ? "SFX: OFF" : "SFX: ON";
        // 3-item menu: Resume | SFX | Quit
        const char* items[3] = {"Resume", sfx_label, "Quit to Menu"};
        for (int i = 0; i < 3; i++) {
            const bool  sel = (focused == i);
            const Color col = sel ? CLRS_ACCENT : CLRS_TEXT_MAIN;
            const char* buf = TextFormat("%s%s", sel ? "> " : "  ", items[i]);
            const Vector2 sz = MeasureTextEx(font_hud_, buf, 24, 1);
            const float iy  = pcy - (3 - 1) * 22.f + i * 44.f;
            DrawTextEx(font_hud_, buf, {pcx - sz.x * 0.5f, iy}, 24, 1, col);
        }
        const char* hint = "up/down: navigate   enter: confirm   esc: resume";
        const Vector2 hs = MeasureTextEx(font_small_, hint, 18, 1);
        DrawTextEx(font_small_, hint, {pcx - hs.x * 0.5f, pcy + 160.f}, 18, 1, CLRS_TEXT_DIM);

    } else {  // CONFIRM_QUIT
        const char* msg = "Quit to main menu?";
        const Vector2 ms = MeasureTextEx(font_hud_, msg, 24, 1);
        DrawTextEx(font_hud_, msg, {pcx - ms.x * 0.5f, pcy - 90.f}, 24, 1, CLRS_TEXT_MAIN);

        const char* items[2] = {"No", "Yes"};
        for (int i = 0; i < 2; i++) {
            const bool  sel = (confirm_focused == i);
            const Color col = sel ? CLRS_ACCENT : CLRS_TEXT_MAIN;
            const char* buf = TextFormat("%s%s", sel ? "> " : "  ", items[i]);
            const Vector2 sz = MeasureTextEx(font_hud_, buf, 24, 1);
            DrawTextEx(font_hud_, buf, {pcx - sz.x * 0.5f, pcy - 10.f + i * 44.f}, 24, 1, col);
        }
        const char* hint = "up/down: navigate   enter: confirm   esc: back";
        const Vector2 hs = MeasureTextEx(font_small_, hint, 18, 1);
        DrawTextEx(font_small_, hint, {pcx - hs.x * 0.5f, pcy + 100.f}, 18, 1, CLRS_TEXT_DIM);
    }
}

// ---------------------------------------------------------------------------
// Classifica fine livello
// ---------------------------------------------------------------------------
void Renderer::DrawResultsScreen(bool in_results, bool local_ready,
                                  const ResultEntry* entries, uint8_t count, uint8_t level,
                                  double elapsed_since_start, double total_duration,
                                  bool coop_all_finished) {
    if (!in_results) return;

    const float rw  = static_cast<float>(GetScreenWidth());
    const float rh  = static_cast<float>(GetScreenHeight());
    const float rcx = rw * 0.5f;
    DrawRectangle(0, 0, (int)rw, (int)rh, CLRS_BG_RESULTS);

    const char* r_title = coop_all_finished ? "LEVEL CLEARED" : "LEVEL FAILED";
    const Color r_tcol  = coop_all_finished ? CLRS_RESULTS_TITLE : CLRS_ERROR;
    const Vector2 r_tsz = MeasureTextEx(font_timer_, r_title, 48, 1);
    DrawTextEx(font_timer_, r_title, {rcx - r_tsz.x * 0.5f, 40.f}, 48, 1, r_tcol);

    static const Color r_rank_col[3] = {CLRS_RESULTS_GOLD, CLRS_RESULTS_SILVER, CLRS_RESULTS_BRONZE};
    const float r_board_y = 120.f;
    for (int ri = 0; ri < (int)count; ri++) {
        const ResultEntry& re = entries[ri];
        const Color        rc = (ri < 3) ? r_rank_col[ri] : CLRS_TEXT_MAIN;
        const float        ry = r_board_y + ri * 44.f + (ri >= 3 ? 20.f : 0.f);
        DrawTextEx(font_hud_, TextFormat("#%d", ri + 1), {rcx - 230.f, ry}, 24, 1, rc);
        DrawTextEx(font_hud_, re.name[0] ? re.name : "?", {rcx - 180.f, ry}, 24, 1, rc);
        const char* r_time;
        if (re.finished) {
            const uint32_t r_cs = re.level_ticks * 100 / 60;
            r_time = TextFormat("%02u:%02u.%02u",
                r_cs / 6000, (r_cs % 6000) / 100, r_cs % 100);
        } else {
            r_time = "DNF";
        }
        const Vector2 r_tsz2 = MeasureTextEx(font_hud_, r_time, 24, 1);
        DrawTextEx(font_hud_, r_time, {rcx + 180.f - r_tsz2.x, ry}, 24, 1, rc);
    }

    const int r_remain = static_cast<int>(total_duration - elapsed_since_start);
    if (r_remain >= 0) {
        const char*   r_cd  = TextFormat("%d", r_remain);
        const Vector2 r_csz = MeasureTextEx(font_timer_, r_cd, 48, 1);
        DrawTextEx(font_timer_, r_cd, {rw - r_csz.x - 20.f, 10.f}, 48, 1, CLRS_TEXT_SOFT_WHITE);
    }

    const char* r_btn  = local_ready ? "Waiting for others..." : "Press JUMP to ready up";
    const Color r_bcol = local_ready ? CLRS_RESULTS_READY : CLRS_ACCENT;
    const Vector2 r_bsz = MeasureTextEx(font_hud_, r_btn, 24, 1);
    DrawTextEx(font_hud_, r_btn, {rcx - r_bsz.x * 0.5f, rh - 60.f}, 24, 1, r_bcol);
}

// ---------------------------------------------------------------------------
// Classifica globale di fine sessione
// ---------------------------------------------------------------------------
void Renderer::DrawGlobalResultsScreen(bool in_global, bool local_ready,
                                        const GlobalResultEntry* entries, uint8_t count,
                                        uint8_t total_levels,
                                        double elapsed_since_start, double total_duration,
                                        uint8_t coop_wins) {
    if (!in_global) return;

    const float rw  = static_cast<float>(GetScreenWidth());
    const float rh  = static_cast<float>(GetScreenHeight());
    const float rcx = rw * 0.5f;
    DrawRectangle(0, 0, (int)rw, (int)rh, CLRS_BG_RESULTS);

    const bool  victory  = (coop_wins * 2 > total_levels);
    const char* verdict  = victory ? "VICTORY!" : "DEFEAT!";
    const Color vcol     = victory ? CLRS_SESSION_OK : CLRS_ERROR;
    const Vector2 v_tsz  = MeasureTextEx(font_timer_, verdict, 72, 1);
    DrawTextEx(font_timer_, verdict, {rcx - v_tsz.x * 0.5f, 30.f}, 72, 1, vcol);

    const char* g_sub = TextFormat("Levels cleared: %u / %u", (unsigned)coop_wins, (unsigned)total_levels);
    const Vector2 g_ssz = MeasureTextEx(font_hud_, g_sub, 26, 1);
    DrawTextEx(font_hud_, g_sub, {rcx - g_ssz.x * 0.5f, 116.f}, 26, 1, CLRS_TEXT_DIM);

    // Player name list
    const float row_start_y = 168.f;
    DrawLineEx({rcx - 200.f, row_start_y - 4.f}, {rcx + 200.f, row_start_y - 4.f}, 1.f, CLRS_TEXT_DIM);
    for (int gi = 0; gi < (int)count; gi++) {
        const GlobalResultEntry& ge = entries[gi];
        const float gy = row_start_y + gi * 40.f;
        const char* name_str = ge.name[0] ? ge.name : "?";
        const Vector2 n_sz = MeasureTextEx(font_hud_, name_str, 26, 1);
        DrawTextEx(font_hud_, name_str, {rcx - n_sz.x * 0.5f, gy}, 26, 1, CLRS_TEXT_MAIN);
    }

    // Countdown
    const int g_remain = static_cast<int>(total_duration - elapsed_since_start);
    if (g_remain >= 0) {
        const char*   g_cd  = TextFormat("%d", g_remain);
        const Vector2 g_csz = MeasureTextEx(font_timer_, g_cd, 48, 1);
        DrawTextEx(font_timer_, g_cd, {rw - g_csz.x - 20.f, 10.f}, 48, 1, CLRS_TEXT_SOFT_WHITE);
    }

    // Pulsante ready
    const char* g_btn  = local_ready ? "Waiting for others..." : "Press JUMP to continue";
    const Color g_bcol = local_ready ? CLRS_RESULTS_READY : CLRS_ACCENT;
    const Vector2 g_bsz = MeasureTextEx(font_hud_, g_btn, 24, 1);
    DrawTextEx(font_hud_, g_btn, {rcx - g_bsz.x * 0.5f, rh - 60.f}, 24, 1, g_bcol);
}

// ---------------------------------------------------------------------------
// Loading overlay — server is generating the next level
// ---------------------------------------------------------------------------
void Renderer::DrawGeneratingLevel(uint8_t level_num, float elapsed_secs) {
    const float sw = static_cast<float>(GetScreenWidth());
    const float sh = static_cast<float>(GetScreenHeight());
    const float cx = sw * 0.5f;
    const float cy = sh * 0.5f;

    // Dark semi-transparent full-screen background
    DrawRectangle(0, 0, (int)sw, (int)sh, {0, 0, 0, 210});

    // Title
    const char* title = level_num > 0
        ? TextFormat("Generating Level %u", (unsigned)level_num)
        : "Generating next level";
    const float   title_sz  = 48.f;
    const Vector2 title_ts  = MeasureTextEx(font_bold_, title, title_sz, 1);
    DrawTextEx(font_bold_, title,
               {cx - title_ts.x * 0.5f, cy - title_sz - 24.f},
               title_sz, 1.f, CLRS_TEXT_MAIN);

    // Animated dots (3 dots cycling every 0.5 s)
    const int dots = static_cast<int>(elapsed_secs / 0.5f) % 4;
    const char* dot_str = dots == 0 ? "   " : dots == 1 ? ".  " : dots == 2 ? ".. " : "...";
    const float   dot_sz = 40.f;
    const Vector2 dot_ts = MeasureTextEx(font_bold_, "...", dot_sz, 1);
    DrawTextEx(font_bold_, dot_str,
               {cx - dot_ts.x * 0.5f, cy + 8.f},
               dot_sz, 1.f, CLRS_ACCENT);

    // Hint at the bottom
    const char* hint = "please wait";
    const Vector2 hs  = MeasureTextEx(font_small_, hint, 18, 1);
    DrawTextEx(font_small_, hint, {cx - hs.x * 0.5f, cy + 70.f}, 18, 1, CLRS_TEXT_DIM);
}

// ---------------------------------------------------------------------------
// Schermate di errore
// ---------------------------------------------------------------------------
void Renderer::DrawConnectionErrorScreen(const char* msg) {
    BeginDrawing();
    ClearBackground(CLRS_BG_GAME);
    const Vector2 ms = MeasureTextEx(font_hud_, msg, 24, 1);
    DrawTextEx(font_hud_, msg,
        {GetScreenWidth() * 0.5f - ms.x * 0.5f,
         GetScreenHeight() * 0.5f - 12.f},
        24, 1, CLRS_ERROR);
    EndDrawing();
}

void Renderer::DrawSessionEndScreen(const char* main_msg, const char* sub_msg, Color col) {
    BeginDrawing();
    ClearBackground(CLRS_BG_GAME);
    const float cx = GetScreenWidth()  * 0.5f;
    const float cy = GetScreenHeight() * 0.5f;
    const Vector2 ms = MeasureTextEx(font_hud_, main_msg, 24, 1);
    DrawTextEx(font_hud_, main_msg, {cx - ms.x * 0.5f, cy - 22.f}, 24, 1, col);
    if (sub_msg && sub_msg[0] != '\0') {
        const Vector2 ms2 = MeasureTextEx(font_small_, sub_msg, 18, 1);
        DrawTextEx(font_small_, sub_msg,
            {cx - ms2.x * 0.5f, cy + 8.f}, 18, 1, CLRS_SESSION_SUB);
    }
    EndDrawing();
}
