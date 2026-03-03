#pragma once
// SRP: loads maps, calculates the optimal spawn point, builds canonical level paths.
// No ENet or session-logic dependency.
#include "World.h"
#include <string>
#include <cstdint>

class LevelManager {
public:
    // Load map from path. Returns false if the file cannot be read.
    bool Load(const char* path);

    // Build the canonical path for a level number (e.g. 2 → "assets/levels/level_02.txt").
    static std::string BuildPath(int num);

    float        SpawnX()   const { return spawn_x_; }
    float        SpawnY()   const { return spawn_y_; }
    const World& GetWorld() const { return world_; }

private:
    World world_;
    float spawn_x_ = 0.f;
    float spawn_y_ = 0.f;
};
