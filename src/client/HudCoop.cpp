// HudCoop.cpp — co-op mode HUD overlay (same as original DrawHUD).

#include "HudCoop.h"
#include "PlayerState.h"
#include "Colors.h"
#include <raylib.h>

void DrawHudModeCoop(Font& font_hud, const PlayerState& s,
                     uint32_t player_count, bool show_players) {
    (void)s;
    const char* txt = show_players
        ? TextFormat("fps: %d  players: %u", GetFPS(), player_count)
        : TextFormat("fps: %d", GetFPS());
    DrawTextEx(font_hud, txt, {10, 10}, 24, 1, WHITE);
}
