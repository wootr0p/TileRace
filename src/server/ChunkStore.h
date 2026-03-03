#pragma once
// SRP: loads and classifies all chunk TMJ files from a directory at startup.
// Stores parsed chunks (char grids + metadata) in memory for use by LevelGenerator.
// No ENet or Raylib dependency.

#include "World.h"
#include <string>
#include <vector>

// One chunk loaded in memory, parsed from a .tmj file.
struct Chunk {
    int width  = 0;
    int height = 0;

    std::vector<std::string>       rows;       // char grid ('0','E','K','X','C','I','O',' ')
    std::vector<std::vector<bool>> solid_grid; // parallel solid-flag grid

    // Entry / exit tile positions (-1 if absent)
    int entry_tx = -1, entry_ty = -1;
    int exit_tx  = -1, exit_ty  = -1;

    // Metadata from TMJ map properties
    std::string role;       // "start", "mid", "end", "any"
    int         difficulty = 1;
    int         weight     = 1;

    // Content flags — set during loading
    bool has_spawn = false; // contains 'X'
    bool has_end   = false; // contains 'E'
};

// Loads all .tmj chunk files from a directory and classifies them into
// start, mid, and end pools.
class ChunkStore {
public:
    // Scan `dir` for .tmj files, parse each into a Chunk, classify into pools.
    // Returns true if at least one start, one mid, and one end chunk were found.
    bool LoadFromDirectory(const char* dir);

    const std::vector<Chunk>& StartChunks() const { return start_; }
    const std::vector<Chunk>& MidChunks()   const { return mid_; }
    const std::vector<Chunk>& EndChunks()   const { return end_; }

    // Min / max difficulty found among mid chunks (for difficulty curve mapping).
    int MinMidDifficulty() const { return min_mid_diff_; }
    int MaxMidDifficulty() const { return max_mid_diff_; }

    bool IsReady() const { return !start_.empty() && !mid_.empty() && !end_.empty(); }

private:
    // Parse a single .tmj chunk file. Returns false on failure.
    bool ParseChunk(const char* path, Chunk& out);

    // Classify chunk into start/mid/end pools based on role + content.
    void Classify(Chunk chunk);

    std::vector<Chunk> start_;
    std::vector<Chunk> mid_;
    std::vector<Chunk> end_;
    int min_mid_diff_ = 1;
    int max_mid_diff_ = 1;
};
