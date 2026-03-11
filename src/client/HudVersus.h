#pragma once
// HUD rendering for versus game mode.
#include <raylib.h>
#include <cstdint>

struct PlayerState;

void DrawHudModeVersus(Font& font_hud, const PlayerState& s,
                       uint32_t player_count, bool show_players);
