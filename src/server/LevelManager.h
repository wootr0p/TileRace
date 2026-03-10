#pragma once
// SRP: loads maps, calculates the optimal spawn point, builds canonical level paths.
// Supports both file-based loading and chunk-based level generation.
// No ENet or session-logic dependency.
#include "World.h"
#include "ChunkStore.h"
#include "LevelGenerator.h"
#include <string>
#include <cstdint>

class LevelManager {
public:
    // Load map from path. Returns false if the file cannot be read.
    bool Load(const char* path);

    // Generate a level from chunks. Returns false on failure.
    // When validate=false, the physics-based level validator is skipped (online mode).
    // total_levels drives the difficulty ramp horizon used by LevelGenerator.
    bool Generate(int level_num, const ChunkStore& store, uint32_t seed = 0,
                  bool validate = true,
                  int total_levels = DIFFICULTY_CURVE_LEVELS);

    // Build the canonical path for a level number (e.g. 2 → "assets/levels/tilemaps/Level02.tmj").
    static std::string BuildPath(int num);

    float        SpawnX()   const { return spawn_x_; }
    float        SpawnY()   const { return spawn_y_; }
    const World& GetWorld() const { return world_; }
    World&       GetWorldMut()    { return world_; }  // mutable access for mode-specific post-processing

private:
    World world_;
    float spawn_x_ = 0.f;
    float spawn_y_ = 0.f;
};
