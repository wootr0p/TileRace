#pragma once
// Header-only spawn utility shared by client (GameSession) and server (LevelManager).
// No Raylib or ENet dependency — only World.h and Physics.h.
#include "World.h"
#include "Physics.h"
#include <cstdlib>   // std::abs (int)
#include <climits>   // INT_MAX
#include <vector>
#include <utility>   // std::pair

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

// Returns the best spawn position for the checkpoint group that contains (ref_tx, ref_ty).
// Flood-fills all 'C' tiles 4-connected to the seed tile, then applies the same
// center + lowest-with-solid-floor logic as FindCenterSpawn.
inline SpawnPos FindCenterCheckpoint(const World& world, int ref_tx, int ref_ty) {
    if (world.GetTile(ref_tx, ref_ty) != 'C') return {};

    const int W = world.GetWidth();
    const int H = world.GetHeight();

    // Flood-fill via explicit stack (no <stack> needed).
    std::vector<std::vector<bool>> visited(H, std::vector<bool>(W, false));
    std::vector<std::pair<int,int>> stk;
    stk.push_back({ref_tx, ref_ty});
    visited[ref_ty][ref_tx] = true;

    std::vector<std::pair<int,int>> group;
    const int ndx[] = {1, -1, 0,  0};
    const int ndy[] = {0,  0, 1, -1};
    while (!stk.empty()) {
        auto [tx, ty] = stk.back(); stk.pop_back();
        group.push_back({tx, ty});
        for (int d = 0; d < 4; ++d) {
            const int nx = tx + ndx[d];
            const int ny = ty + ndy[d];
            if (nx >= 0 && nx < W && ny >= 0 && ny < H &&
                !visited[ny][nx] && world.GetTile(nx, ny) == 'C') {
                visited[ny][nx] = true;
                stk.push_back({nx, ny});
            }
        }
    }

    // Horizontal mean of all tiles in the group.
    int sum_tx = 0;
    for (const auto& [tx, ty] : group) sum_tx += tx;
    const int mean_tx = sum_tx / static_cast<int>(group.size());

    // Lowest tile that has solid floor directly beneath it.
    int best_ty = -1;
    for (const auto& [tx, ty] : group)
        if (world.IsSolid(tx, ty + 1) && ty > best_ty)
            best_ty = ty;

    // Fallback: if no tile has solid floor beneath, pick the lowest tile.
    if (best_ty == -1)
        for (const auto& [tx, ty] : group)
            if (ty > best_ty) best_ty = ty;

    // Among tiles at best_ty, pick the one closest to the horizontal mean.
    int best_tx   = mean_tx;
    int best_dist = INT_MAX;
    for (const auto& [tx, ty] : group) {
        if (ty == best_ty) {
            const int d = std::abs(tx - mean_tx);
            if (d < best_dist) { best_dist = d; best_tx = tx; }
        }
    }

    return { static_cast<float>(best_tx * TILE_SIZE),
             static_cast<float>(best_ty * TILE_SIZE) };
}
