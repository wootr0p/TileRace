// SessionResultsRace.cpp — race mode session-end global results screen.
// Shows leaderboard sorted by wins (most wins first) with win counts.

#include "SessionResultsRace.h"
#include "Protocol.h"
#include "Colors.h"
#include <raylib.h>
#include <algorithm>

void DrawSessionResultsModeRace(Font& font_hud, Font& font_timer,
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

    // "Race Mode" header
    const char* mode_lbl = "Race Mode";
    const Vector2 ml_sz = MeasureTextEx(font_timer, mode_lbl, 32, 1);
    DrawTextEx(font_timer, mode_lbl, {rcx - ml_sz.x * 0.5f, 10.f}, 32, 1, CLRS_ACCENT);

    // Title
    const char* title = "LEADERBOARD";
    const Vector2 t_sz = MeasureTextEx(font_timer, title, 52, 1);
    DrawTextEx(font_timer, title, {rcx - t_sz.x * 0.5f, 50.f}, 52, 1, WHITE);

    const char* g_sub = TextFormat("Total levels: %u", (unsigned)total_levels);
    const Vector2 g_ssz = MeasureTextEx(font_hud, g_sub, 26, 1);
    DrawTextEx(font_hud, g_sub, {rcx - g_ssz.x * 0.5f, 116.f}, 26, 1, CLRS_TEXT_DIM);

    // Sort entries by wins descending
    GlobalResultEntry sorted[MAX_PLAYERS];
    const int n = (count <= MAX_PLAYERS) ? count : MAX_PLAYERS;
    for (int i = 0; i < n; i++) sorted[i] = entries[i];
    std::sort(sorted, sorted + n, [](const GlobalResultEntry& a, const GlobalResultEntry& b) {
        return a.wins > b.wins;
    });

    // Draw ranked list
    const float row_start_y = 160.f;
    DrawLineEx({rcx - 220.f, row_start_y - 4.f}, {rcx + 220.f, row_start_y - 4.f}, 1.f, CLRS_TEXT_DIM);
    for (int gi = 0; gi < n; gi++) {
        const GlobalResultEntry& ge = sorted[gi];
        const float gy = row_start_y + gi * 40.f;
        const char* name_str = ge.name[0] ? ge.name : "?";
        const char* rank_str = TextFormat("#%d", gi + 1);
        const char* wins_str = TextFormat("%u wins", (unsigned)ge.wins);

        // Rank number (left aligned)
        const Color rank_col = (gi == 0) ? CLRS_ACCENT : CLRS_TEXT_MAIN;
        DrawTextEx(font_hud, rank_str, {rcx - 200.f, gy}, 26, 1, rank_col);

        // Name (center-left)
        DrawTextEx(font_hud, name_str, {rcx - 130.f, gy}, 26, 1, rank_col);

        // Wins (right aligned)
        const Vector2 w_sz = MeasureTextEx(font_hud, wins_str, 26, 1);
        DrawTextEx(font_hud, wins_str, {rcx + 200.f - w_sz.x, gy}, 26, 1, CLRS_TEXT_DIM);
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
