#pragma once
// SRP: composes a playable level from chunk pieces stored in ChunkStore.
// Algorithm: 1 start + N mid + 1 end, stitched via entry/exit tile alignment.
// Adds a 5-tile solid border + 5-tile empty margin around the composed map.
// No ENet or Raylib dependency.

#include "ChunkStore.h"
#include "World.h"
#include <cstdint>

struct GeneratorParams {
    int level_num    = 1;    // current level number (1-based)
    int total_levels = 8;    // total levels in the session (for difficulty curve)
    uint32_t seed    = 0;    // RNG seed (0 = use current time)
};

class LevelGenerator {
public:
    // Generate a level from the chunks in `store` and load it into `world`.
    // Returns true on success.
    static bool Generate(const ChunkStore& store, const GeneratorParams& params, World& world);

private:
    // Choose how many mid chunks based on level progression.
    static int MidChunkCount(int level_num, int total_levels);

    // Compute the target difficulty [0.0, 1.0] → mapped to the store's range.
    static float TargetDifficulty(int level_num, int total_levels);
};
