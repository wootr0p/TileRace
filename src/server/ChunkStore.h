#pragma once
// SRP: loads and classifies all chunk TMJ files from a directory at startup.
// Stores parsed chunks (char grids + metadata) in memory for use by LevelGenerator.
// No ENet or Raylib dependency.

#include "World.h"
#include <string>
#include <vector>
#include <utility>

// One chunk loaded in memory, parsed from a .tmj file.
struct Chunk {
    int width  = 0;
    int height = 0;

    std::vector<std::string>       rows;       // char grid ('0','E','K','X','C','I','O',' ')
    std::vector<std::vector<bool>> solid_grid; // parallel solid-flag grid

    // Primary entry / exit tile positions (-1 if absent).
    // These are the first 'I' and 'O' tiles found during scanning
    // and remain for backward compatibility with single-entry/exit chunks.
    int entry_tx = -1, entry_ty = -1;
    int exit_tx  = -1, exit_ty  = -1;

    // All entry ('I') and exit ('O') tile positions.
    // Used to detect fork chunks: multiple exits = fork start, multiple entries = fork end.
    std::vector<std::pair<int,int>> entries;  // all 'I' tile positions (tx, ty)
    std::vector<std::pair<int,int>> exits;    // all 'O' tile positions (tx, ty)

    // Metadata from TMJ map properties
    std::string role;       // "start", "mid", "end", "any"
    int         difficulty = 1;
    int         weight     = 1;

    // Content flags — set during loading
    bool has_spawn      = false; // contains 'X'
    bool has_end        = false; // contains 'E'
    bool has_checkpoint = false; // contains 'C'

    // Fork detection helpers
    bool IsForkStart() const { return exits.size() > 1; }
    bool IsForkEnd()   const { return entries.size() > 1; }
    int  ExitCount()   const { return static_cast<int>(exits.size()); }
    int  EntryCount()  const { return static_cast<int>(entries.size()); }
};

// Loads all .tmj chunk files from a directory and classifies them into
// start, mid, end, fork_start, and fork_end pools.
class ChunkStore {
public:
    // Scan `dir` for .tmj files, parse each into a Chunk, classify into pools.
    // Returns true if at least one start, one mid, and one end chunk were found.
    bool LoadFromDirectory(const char* dir);

    const std::vector<Chunk>& StartChunks()     const { return start_; }
    const std::vector<Chunk>& MidChunks()       const { return mid_; }
    const std::vector<Chunk>& EndChunks()       const { return end_; }
    const std::vector<Chunk>& ForkStartChunks() const { return fork_start_; }
    const std::vector<Chunk>& ForkEndChunks()   const { return fork_end_; }

    // Min / max difficulty found among mid chunks (for difficulty curve mapping).
    int MinMidDifficulty() const { return min_mid_diff_; }
    int MaxMidDifficulty() const { return max_mid_diff_; }

    bool IsReady() const { return !start_.empty() && !mid_.empty() && !end_.empty(); }

    // True if fork chunks are available (both fork-start and fork-end).
    bool HasForkChunks() const { return !fork_start_.empty(); }

    // Mid-chunk sub-pools based on checkpoint content.
    const std::vector<Chunk>& MidCheckpointChunks() const { return mid_checkpoint_; }
    const std::vector<Chunk>& MidNormalChunks()      const { return mid_normal_; }
    bool HasMidCheckpointChunks() const { return !mid_checkpoint_.empty(); }

private:
    // Parse a single .tmj chunk file. Returns false on failure.
    bool ParseChunk(const char* path, Chunk& out);

    // Classify chunk into start/mid/end pools based on role + content.
    void Classify(Chunk chunk);

    // After all chunks are loaded, scan for fork chunks (multi-exit / multi-entry).
    void DetectForkChunks();

    std::vector<Chunk> start_;
    std::vector<Chunk> mid_;
    std::vector<Chunk> end_;
    std::vector<Chunk> fork_start_;      // chunks with multiple exits (fork entry points)
    std::vector<Chunk> fork_end_;        // chunks with multiple entries (fork merge points)
    std::vector<Chunk> mid_checkpoint_;  // mid chunks that contain 'C' tiles
    std::vector<Chunk> mid_normal_;      // mid chunks without 'C' tiles
    int min_mid_diff_ = 1;
    int max_mid_diff_ = 1;
};
