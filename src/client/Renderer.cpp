#include "Renderer.h"
#include "../common/Physics.h"
#include <raylib.h>
#include <algorithm>

// Nota: includiamo Physics.h per TILE_SIZE — ma è un header del /common senza Raylib.
// Questo è accettabile: il Renderer può conoscere le costanti di gioco.
// Non può però MAI modificare lo stato di gioco.

// ---- Palette neon su sfondo quasi-nero ----
static constexpr Color COL_BG          = {  5,   5,  15, 255 };  // nero-blu profondo
static constexpr Color COL_TILE        = { 20, 110, 130, 255 };  // teal scuro
static constexpr Color COL_TILE_BORDER = {  0, 200, 220, 255 };  // bordo ciano neon
static constexpr Color COL_PLAYER_ME   = {  0, 255, 180, 255 };  // ciano-verde neon
static constexpr Color COL_PLAYER_OTH  = {255,  50, 200, 255 };  // magenta neon
static constexpr Color COL_HUD         = {  0, 230, 200, 255 };  // ciano chiaro
static constexpr Color COL_HUD_DIM     = { 80, 160, 160, 255 };  // ciano dimmed
static constexpr Color COL_GROUND_IND  = {  0, 255, 100, 255 };  // verde neon
static constexpr Color COL_AIR_IND     = {255, 180,   0, 255 };  // arancio neon

bool Renderer::Init(int width, int height, const char* title) {
    screen_w_ = width;
    screen_h_ = height;
    // Se la finestra è già aperta (es. aperta da main.cpp per il menu),
    // non la ricrea: Raylib ha una sola finestra per processo.
    if (!IsWindowReady()) {
        InitWindow(width, height, title);
        SetTargetFPS(144);
    }
    return IsWindowReady();
}

void Renderer::Shutdown() {
    // La finestra viene chiusa da chi l'ha aperta (main.cpp),
    // quindi qui non chiamiamo CloseWindow.
}

bool Renderer::ShouldClose() const {
    return WindowShouldClose();
}

void Renderer::Draw(const GameState& state, const World& world,
                    uint32_t local_player_id, float cam_x, float cam_y) const {
    // DEBUG: camera fissa che inquadra l'intero livello.
    // Per tornare alla camera che segue il player, rimpiazza questo blocco
    // con: target={cam_x,cam_y}, zoom=2.0f
    float world_pw = world.GetWidth()  * Physics::TILE_SIZE;
    float world_ph = world.GetHeight() * Physics::TILE_SIZE;
    float zoom = std::min(screen_w_ / world_pw, screen_h_ / world_ph);

    Camera2D camera = {};
    camera.offset   = { screen_w_ / 2.0f, screen_h_ / 2.0f };
    camera.target   = { world_pw / 2.0f,  world_ph / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom     = zoom;

    BeginDrawing();
    ClearBackground(COL_BG);

    // World space: tilemap e giocatori trasformati dalla camera
    BeginMode2D(camera);
    DrawWorld(world);
    DrawPlayers(state, local_player_id);
    EndMode2D();

    // Screen space: HUD sempre in coordinate fisse, sopra tutto
    DrawHUD(state, local_player_id);

    EndDrawing();
}

void Renderer::DrawWorld(const World& world) const {
    const int ts = static_cast<int>(Physics::TILE_SIZE);
    for (int ty = 0; ty < world.GetHeight(); ++ty) {
        for (int tx = 0; tx < world.GetWidth(); ++tx) {
            if (!world.IsSolid(tx, ty)) continue;
            int px = tx * ts;
            int py = ty * ts;
            // Fill pieno — nessuno spazio tra i tile
            DrawRectangle(px, py, ts, ts, COL_TILE);
            // Bordo interno 1px per dare profondità senza distanza
            DrawRectangleLines(px, py, ts, ts, COL_TILE_BORDER);
        }
    }
}

void Renderer::DrawPlayers(const GameState& state, uint32_t local_player_id) const {
    for (int i = 0; i < state.player_count; ++i) {
        const PlayerState& ps = state.players[i];
        bool is_me = (ps.player_id == local_player_id);
        Color body  = is_me ? COL_PLAYER_ME : COL_PLAYER_OTH;
        Color glow  = is_me ? Color{0, 255, 180, 60} : Color{255, 50, 200, 60};

        int px = static_cast<int>(ps.x);
        int py = static_cast<int>(ps.y);

        // Alone (glow): rettangolo leggermente più grande
        DrawRectangle(px - 2, py - 2, 36, 36, glow);
        // Corpo 32x32
        DrawRectangle(px, py, 32, 32, body);
        // Bordo neon
        DrawRectangleLines(px, py, 32, 32, WHITE);

        // ID sopra il giocatore
        DrawText(
            TextFormat("%u", ps.player_id),
            px + 2,
            py - 14,
            10,
            body
        );
    }
}

void Renderer::DrawHUD(const GameState& state, uint32_t local_player_id) const {
    DrawText(TextFormat("Tick server: %u", state.server_tick), 10, 10, 18, COL_HUD_DIM);
    DrawText(TextFormat("Giocatori: %d",   state.player_count), 10, 30, 18, COL_HUD_DIM);
    DrawText(TextFormat("FPS: %d",         GetFPS()),           10, 50, 18, COL_HUD);

    const PlayerState* local = state.FindPlayer(local_player_id);
    if (local) {
        DrawText(TextFormat("Pos: %.1f, %.1f", local->x, local->y),         10,  70, 18, COL_HUD);
        DrawText(TextFormat("Vel: %.1f, %.1f", local->vel_x, local->vel_y), 10,  90, 18, COL_HUD);
        DrawText(local->on_ground ? "A terra" : "In aria",                  10, 110, 18,
                 local->on_ground ? COL_GROUND_IND : COL_AIR_IND);
    }
}
