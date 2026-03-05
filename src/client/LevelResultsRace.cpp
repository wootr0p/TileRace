// LevelResultsRace.cpp — race mode end-of-level results screen.
// Same as co-op but with "Race Mode" header.

#include "LevelResultsRace.h"
#include "Protocol.h"
#include "Colors.h"
#include <raylib.h>

void DrawLevelResultsModeRace(Font& font_hud, Font& font_timer,
                              bool in_results, bool local_ready,
                              const ResultEntry* entries, uint8_t count, uint8_t level,
                              double elapsed_since_start, double total_duration,
                              bool coop_all_finished) {
    if (!in_results) return;
    (void)level;
    (void)coop_all_finished;

    const float rw  = static_cast<float>(GetScreenWidth());
    const float rh  = static_cast<float>(GetScreenHeight());
    const float rcx = rw * 0.5f;
    DrawRectangle(0, 0, (int)rw, (int)rh, CLRS_BG_RESULTS);

    // "Race Mode" header
    const char* mode_lbl = "Race Mode";
    const Vector2 ml_sz = MeasureTextEx(font_timer, mode_lbl, 32, 1);
    DrawTextEx(font_timer, mode_lbl, {rcx - ml_sz.x * 0.5f, 10.f}, 32, 1, CLRS_ACCENT);

    // Title
    const char* r_title = "LEVEL RESULTS";
    const Color r_tcol  = CLRS_RESULTS_TITLE;
    const Vector2 r_tsz = MeasureTextEx(font_timer, r_title, 48, 1);
    DrawTextEx(font_timer, r_title, {rcx - r_tsz.x * 0.5f, 50.f}, 48, 1, r_tcol);

    static const Color r_rank_col[3] = {CLRS_RESULTS_GOLD, CLRS_RESULTS_SILVER, CLRS_RESULTS_BRONZE};
    const float r_board_y = 130.f;
    for (int ri = 0; ri < (int)count; ri++) {
        const ResultEntry& re = entries[ri];
        const Color        rc = (ri < 3) ? r_rank_col[ri] : CLRS_TEXT_MAIN;
        const float        ry = r_board_y + ri * 44.f + (ri >= 3 ? 20.f : 0.f);
        DrawTextEx(font_hud, TextFormat("#%d", ri + 1), {rcx - 230.f, ry}, 24, 1, rc);
        DrawTextEx(font_hud, re.name[0] ? re.name : "?", {rcx - 180.f, ry}, 24, 1, rc);
        const char* r_time;
        if (re.finished) {
            const uint32_t r_cs = re.level_ticks * 100 / 60;
            r_time = TextFormat("%02u:%02u.%02u",
                r_cs / 6000, (r_cs % 6000) / 100, r_cs % 100);
        } else {
            r_time = "DNF";
        }
        const Vector2 r_tsz2 = MeasureTextEx(font_hud, r_time, 24, 1);
        DrawTextEx(font_hud, r_time, {rcx + 180.f - r_tsz2.x, ry}, 24, 1, rc);
    }

    const int r_remain = static_cast<int>(total_duration - elapsed_since_start);
    if (r_remain >= 0) {
        const char*   r_cd  = TextFormat("%d", r_remain);
        const Vector2 r_csz = MeasureTextEx(font_timer, r_cd, 48, 1);
        DrawTextEx(font_timer, r_cd, {rw - r_csz.x - 20.f, 10.f}, 48, 1, CLRS_TEXT_SOFT_WHITE);
    }

    const char* r_btn  = local_ready ? "Waiting for others..." : "Press JUMP to ready up";
    const Color r_bcol = local_ready ? CLRS_RESULTS_READY : CLRS_ACCENT;
    const Vector2 r_bsz = MeasureTextEx(font_hud, r_btn, 24, 1);
    DrawTextEx(font_hud, r_btn, {rcx - r_bsz.x * 0.5f, rh - 60.f}, 24, 1, r_bcol);
}
