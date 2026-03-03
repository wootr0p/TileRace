#pragma once
#include <string>
#include <vector>

// Tilemap loaded from either:
//   • a plain-text .txt file  (legacy format, kept for unit tests)
//   • a Tiled JSON map   .tmj file  (current format)
//
// Recognised tile chars (stored internally regardless of source format):
//   '0' = wall / platform  (solid)
//   'E' = exit             (non-solid; win condition on touch)
//   'K' = kill             (non-solid by default; touching respawns the player)
//   'X' = spawn            (non-solid; player start position)
//   ' ' = air
//
// For .tmj files the solid flag comes from the TileSet.tsx "solid" property,
// so each tile type can independently be solid or non-solid.
class World {
public:
    // Returns true if the file was opened and parsed successfully.
    // Accepts both .txt (legacy ASCII) and .tmj (Tiled JSON) paths.
    bool LoadFromFile(const char* path);

    // Load from an in-memory char grid. solid_grid is reconstructed as (ch == '0').
    // Used by the chunk-based level generator to load generated levels on both
    // server and client without writing temporary files.
    bool LoadFromGrid(int w, int h, const std::vector<std::string>& rows);

    // Returns true if tile (tx, ty) should block player movement.
    // For .txt: solid iff char == '0'.
    // For .tmj: solid iff the TSX "solid" property is true for that tile type.
    // Out-of-bounds coords always return false.
    bool IsSolid(int tx, int ty) const;

    // Returns the canonical tile char at (tx, ty), or ' ' if out of bounds.
    char GetTile(int tx, int ty) const;

    int GetWidth()  const { return width_; }
    int GetHeight() const { return height_; }

    const std::vector<std::string>&       GetRows()      const { return rows_; }
    const std::vector<std::vector<bool>>& GetSolidGrid() const { return solid_grid_; }

private:
    std::vector<std::string>      rows_;        // char grid ('0','E','K','X',' ')
    std::vector<std::vector<bool>> solid_grid_; // parallel solid-flag grid
    int width_  = 0;
    int height_ = 0;

    bool LoadTxt(const char* path);
    bool LoadTmj(const char* path);
};
