// HudRace.cpp — race mode HUD overlay.
// Same as co-op HUD but with "Race Mode" label at top-right.

#include "HudRace.h"
#include "PlayerState.h"
#include "Colors.h"
#include <raylib.h>

void DrawHudModeRace(Font& font_hud, const PlayerState& s,
                     uint32_t player_count, bool show_players) {
    (void)s;
    const char* txt = show_players
        ? TextFormat("fps: %d  players: %u", GetFPS(), player_count)
        : TextFormat("fps: %d", GetFPS());
    DrawTextEx(font_hud, txt, {10, 10}, 24, 1, WHITE);

    // Race mode indicator (top-right)
    const char* mode_lbl = "Race Mode";
    const float mode_sz  = 20.f;
    const Vector2 ms = MeasureTextEx(font_hud, mode_lbl, mode_sz, 1);
    DrawTextEx(font_hud, mode_lbl,
               {static_cast<float>(GetScreenWidth()) - ms.x - 12.f, 12.f},
               mode_sz, 1, CLRS_ACCENT);
}
