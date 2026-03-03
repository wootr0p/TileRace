#pragma once
#include <string>
#include <vector>

// Tilemap loaded from a plain-text .txt file. No external dependencies.
//
// Recognised tile characters:
//   '0' = wall   (solid)
//   'E' = exit   (non-solid; win condition on touch)
//   'K' = kill   (non-solid; touching respawns the player)
//   'X' = spawn  (non-solid; player start position)
//   ' ' = air
class World {
public:
    // Returns true if the file was opened successfully.
    bool LoadFromFile(const char* path);

    // Returns true if tile (tx, ty) is solid ('0'). Out-of-bounds coords → false.
    bool IsSolid(int tx, int ty) const;

    // Returns the raw tile char at (tx, ty), or ' ' if out of bounds.
    char GetTile(int tx, int ty) const;

    int GetWidth()  const { return width_; }
    int GetHeight() const { return height_; }

private:
    std::vector<std::string> rows_;
    int width_  = 0;
    int height_ = 0;
};
