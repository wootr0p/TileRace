#pragma once
// End-of-level results screen for co-op mode.
#include <raylib.h>
#include <cstdint>

struct ResultEntry;

void DrawLevelResultsModeCoop(Font& font_hud, Font& font_timer,
                              bool in_results, bool local_ready,
                              const ResultEntry* entries, uint8_t count, uint8_t level,
                              double elapsed_since_start, double total_duration,
                              bool coop_all_finished);
