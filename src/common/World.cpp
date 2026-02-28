#include "World.h"
#include <fstream>
#include <algorithm>    // std::max

bool World::LoadFromFile(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    rows_.clear();
    width_ = 0;
    height_ = 0;

    std::string line;
    while (std::getline(file, line)) {
        width_ = std::max(width_, static_cast<int>(line.size()));
        rows_.push_back(std::move(line));
    }

    // Normalizza ogni riga alla larghezza massima (riempie con spazi).
    // Garantisce che GetTile(tx, ty) con tx < GetWidth() sia sempre valido.
    for (auto& row : rows_) {
        if (static_cast<int>(row.size()) < width_)
            row.resize(width_, ' ');
    }

    height_ = static_cast<int>(rows_.size());
    return height_ > 0;
}

bool World::IsSolid(int tx, int ty) const {
    char c = GetTile(tx, ty);
    return c == '0';  // solo muri; 'E' è endpoint non solido (il player ci passa attraverso)
}

char World::GetTile(int tx, int ty) const {
    if (ty < 0 || ty >= height_) return ' ';
    if (tx < 0 || tx >= width_)  return ' ';
    return rows_[ty][tx];
}
