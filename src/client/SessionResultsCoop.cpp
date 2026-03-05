// SessionResultsCoop.cpp — co-op mode session-end global results screen.
// Extracted from the original Renderer::DrawGlobalResultsScreen.

#include "SessionResultsCoop.h"
#include "Protocol.h"
#include "Colors.h"
#include <raylib.h>

void DrawSessionResultsModeCoop(Font& font_hud, Font& font_timer,
                                bool in_global, bool local_ready,
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
    const Vector2 v_tsz  = MeasureTextEx(font_timer, verdict, 72, 1);
    DrawTextEx(font_timer, verdict, {rcx - v_tsz.x * 0.5f, 30.f}, 72, 1, vcol);

    const char* g_sub = TextFormat("Levels cleared: %u / %u", (unsigned)coop_wins, (unsigned)total_levels);
    const Vector2 g_ssz = MeasureTextEx(font_hud, g_sub, 26, 1);
    DrawTextEx(font_hud, g_sub, {rcx - g_ssz.x * 0.5f, 116.f}, 26, 1, CLRS_TEXT_DIM);

    const float row_start_y = 168.f;
    DrawLineEx({rcx - 200.f, row_start_y - 4.f}, {rcx + 200.f, row_start_y - 4.f}, 1.f, CLRS_TEXT_DIM);
    for (int gi = 0; gi < (int)count; gi++) {
        const GlobalResultEntry& ge = entries[gi];
        const float gy = row_start_y + gi * 40.f;
        const char* name_str = ge.name[0] ? ge.name : "?";
        const Vector2 n_sz = MeasureTextEx(font_hud, name_str, 26, 1);
        DrawTextEx(font_hud, name_str, {rcx - n_sz.x * 0.5f, gy}, 26, 1, CLRS_TEXT_MAIN);
    }

    const int g_remain = static_cast<int>(total_duration - elapsed_since_start);
    if (g_remain >= 0) {
        const char*   g_cd  = TextFormat("%d", g_remain);
        const Vector2 g_csz = MeasureTextEx(font_timer, g_cd, 48, 1);
        DrawTextEx(font_timer, g_cd, {rw - g_csz.x - 20.f, 10.f}, 48, 1, CLRS_TEXT_SOFT_WHITE);
    }

    const char* g_btn  = local_ready ? "Waiting for others..." : "Press JUMP to continue";
    const Color g_bcol = local_ready ? CLRS_RESULTS_READY : CLRS_ACCENT;
    const Vector2 g_bsz = MeasureTextEx(font_hud, g_btn, 24, 1);
    DrawTextEx(font_hud, g_btn, {rcx - g_bsz.x * 0.5f, rh - 60.f}, 24, 1, g_bcol);
}
