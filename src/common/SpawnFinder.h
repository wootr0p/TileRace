#pragma once
// Header-only spawn utility shared by client (GameSession) and server (LevelManager).
// No Raylib or ENet dependency — only World.h and Physics.h.
#include "World.h"
#include "Physics.h"
#include <cstdlib>   // std::abs (int)
#include <climits>   // INT_MAX

struct SpawnPos { float x = 0.f, y = 0.f; };

// Returns pixel position (top-left) of the best spawn point among 'X' tiles:
//   x = horizontal centre of all 'X' tiles, snapped to the nearest tile
//   y = lowest 'X' tile that has solid floor directly beneath it
inline SpawnPos FindCenterSpawn(const World& world) {
    int sum_tx  = 0;
    int count   = 0;
    int best_ty = -1;

    for (int ty = 0; ty < world.GetHeight(); ++ty) {
        for (int tx = 0; tx < world.GetWidth(); ++tx) {
            if (world.GetTile(tx, ty) != 'X') continue;
            sum_tx += tx;
            ++count;
            if (world.IsSolid(tx, ty + 1) && ty > best_ty)
                best_ty = ty;
        }
    }

    if (count == 0) return {};

    const int mean_tx = sum_tx / count;

    // Among 'X' tiles at best_ty, pick the one closest to the horizontal mean.
    int best_tx    = mean_tx;
    int best_dist  = INT_MAX;
    for (int tx = 0; tx < world.GetWidth(); ++tx) {
        if (world.GetTile(tx, best_ty) == 'X') {
            const int d = std::abs(tx - mean_tx);
            if (d < best_dist) { best_dist = d; best_tx = tx; }
        }
    }

    return { static_cast<float>(best_tx * TILE_SIZE),
             static_cast<float>(best_ty * TILE_SIZE) };
}
