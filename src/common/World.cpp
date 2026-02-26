#include "World.h"
#include "Physics.h"
#include <fstream>
#include <algorithm>
#include <cmath>

bool World::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    tiles_.clear();
    width_ = 0;
    height_ = 0;

    std::string line;
    while (std::getline(file, line)) {
        std::vector<int> row;
        row.reserve(line.size());
        for (char c : line) {
            // Formato mappa:
            //   '0' = tile solido
            //   ' ' = vuoto
            //   'E' = zona traguardo (solido visivamente, gestito dal gioco)
            //   'X' = spawn point (non solido)
            // Qualsiasi cosa non riconosciuta = vuoto
            row.push_back((c == '0' || c == 'E') ? 1 : 0);
        }
        if (!row.empty()) {
            width_ = std::max(width_, static_cast<int>(row.size()));
            tiles_.push_back(std::move(row));
        }
    }

    height_ = static_cast<int>(tiles_.size());
    return height_ > 0;
}

bool World::IsSolid(int tile_x, int tile_y) const {
    if (tile_x < 0 || tile_y < 0 || tile_y >= height_) return true;  // Bordi = solidi
    if (tile_x >= width_) return false;
    if (tile_x >= static_cast<int>(tiles_[tile_y].size())) return false;
    return tiles_[tile_y][tile_x] != 0;
}

bool World::ResolveCollision(float pos_x, float pos_y, float w, float h,
                              float& out_x, float& out_y) const {
    // Algoritmo: itera tutti i tile che l'AABB interseca e risolve le sovrapposizioni.
    // È un approccio semplice (non continuo) adatto a velocità moderate.
    //
    // NOTA DIDATTICA: questo è un algoritmo "discrete collision detection".
    // A velocità molto alte il corpo può "attraversare" un tile in un tick.
    // Per ora è sufficiente per il nostro platform game.

    out_x = pos_x;
    out_y = pos_y;
    bool collided = false;

    const float ts = Physics::TILE_SIZE;

    // Tile range da controllare
    int left   = static_cast<int>(std::floor(out_x / ts));
    int right  = static_cast<int>(std::floor((out_x + w - 1.0f) / ts));
    int top    = static_cast<int>(std::floor(out_y / ts));
    int bottom = static_cast<int>(std::floor((out_y + h - 1.0f) / ts));

    for (int ty = top; ty <= bottom; ++ty) {
        for (int tx = left; tx <= right; ++tx) {
            if (!IsSolid(tx, ty)) continue;
            collided = true;

            // Calcola la sovrapposizione su X e Y
            float tile_left   = static_cast<float>(tx) * ts;
            float tile_top    = static_cast<float>(ty) * ts;
            float tile_right  = tile_left + ts;
            float tile_bottom = tile_top  + ts;

            float overlap_x = std::min(out_x + w, tile_right)  - std::max(out_x, tile_left);
            float overlap_y = std::min(out_y + h, tile_bottom) - std::max(out_y, tile_top);

            // Risolvi lungo l'asse con sovrapposizione minore (MTV — Minimum Translation Vector)
            if (overlap_x < overlap_y) {
                if (out_x < tile_left) out_x -= overlap_x;
                else                   out_x += overlap_x;
            } else {
                if (out_y < tile_top)  out_y -= overlap_y;
                else                   out_y += overlap_y;
            }

            // Ricalcola i limiti dopo la correzione
            left   = static_cast<int>(std::floor(out_x / ts));
            right  = static_cast<int>(std::floor((out_x + w - 1.0f) / ts));
            top    = static_cast<int>(std::floor(out_y / ts));
            bottom = static_cast<int>(std::floor((out_y + h - 1.0f) / ts));
        }
    }

    return collided;
}
