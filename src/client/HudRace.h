#pragma once
// HUD rendering for race game mode.
#include <raylib.h>
#include <cstdint>

struct PlayerState;

void DrawHudModeRace(Font& font_hud, const PlayerState& s,
                     uint32_t player_count, bool show_players);
