#pragma once
// Session-end global results screen for co-op mode.
#include <raylib.h>
#include <cstdint>

struct GlobalResultEntry;

void DrawSessionResultsModeCoop(Font& font_hud, Font& font_timer,
                                bool in_global, bool local_ready,
                                const GlobalResultEntry* entries, uint8_t count,
                                uint8_t total_levels,
                                double elapsed_since_start, double total_duration,
                                uint8_t coop_wins);
