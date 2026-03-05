#pragma once
// HUD rendering for co-op game mode.
#include <raylib.h>
#include <cstdint>

struct PlayerState;

void DrawHudModeCoop(Font& font_hud, const PlayerState& s,
                     uint32_t player_count, bool show_players);
