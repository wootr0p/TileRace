#include "World.h"
#include <fstream>
#include <sstream>
#include <algorithm>    // std::max
#include <cctype>       // std::isdigit
#include <cstring>      // std::strstr
#include <string>
#include <vector>
#include <unordered_map>

// ============================================================================
// Internal helpers
// ============================================================================

// Returns the directory portion of a file path (e.g. "a/b/c.txt" → "a/b").
static std::string DirOf(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? std::string(".") : path.substr(0, pos);
}

// Reads an entire file into a std::string.
static bool ReadFile(const char* path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

// ============================================================================
// Minimal JSON helpers for .tmj parsing
// (handles the flat Tiled uncompressed orthogonal format only)
// ============================================================================

// Find the first integer value associated with "key" at any depth in `json`.
static bool JsonGetInt(const std::string& json, const std::string& key, int& out) {
    const std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return false;
    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return false;
    try { out = std::stoi(json.substr(pos)); return true; }
    catch (...) { return false; }
}

// Find the first string value (unescaping `\/` → `/`) for "key" in `json`.
static bool JsonGetString(const std::string& json, const std::string& key, std::string& out) {
    const std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return false;
    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) ++pos;
    if (pos >= json.size() || json[pos] != '"') return false;
    ++pos;
    const size_t end = json.find('"', pos);
    if (end == std::string::npos) return false;
    out = json.substr(pos, end - pos);
    // Unescape `\/` → `/` (Tiled path separator escaping)
    for (size_t i = 0; i + 1 < out.size(); ) {
        if (out[i] == '\\' && out[i + 1] == '/') { out.erase(i, 1); }
        else ++i;
    }
    return true;
}

// Extract the tile GID data array from the first "tilelayer" layer.
// Scans the content of the first '[' after the first `"data"` key.
static bool TmjParseData(const std::string& json, std::vector<int>& data) {
    const size_t data_key = json.find("\"data\"");
    if (data_key == std::string::npos) return false;
    const size_t arr_open = json.find('[', data_key);
    if (arr_open == std::string::npos) return false;
    const size_t arr_close = json.find(']', arr_open);
    if (arr_close == std::string::npos) return false;

    size_t cur = arr_open + 1;
    while (cur < arr_close) {
        // Skip whitespace and separators
        while (cur < arr_close &&
               (json[cur] == ' ' || json[cur] == ',' ||
                json[cur] == '\n' || json[cur] == '\r' || json[cur] == '\t')) ++cur;
        if (cur >= arr_close) break;

        // Parse an integer (positive; GIDs are never negative in standard Tiled)
        size_t num_end = cur;
        while (num_end < arr_close && std::isdigit(static_cast<unsigned char>(json[num_end]))) ++num_end;
        if (num_end > cur) {
            try { data.push_back(std::stoi(json.substr(cur, num_end - cur))); }
            catch (...) { data.push_back(0); }
            cur = num_end;
        } else {
            ++cur; // skip unexpected char
        }
    }
    return !data.empty();
}

// ============================================================================
// Minimal TSX (XML) parser for Tiled tilesets
// ============================================================================

struct TileProps {
    std::string type;   // "platform" | "kill" | "end" | "spawn"
    bool        solid = false;
};

// Parse a TileSet.tsx file and return a map of tile_id → TileProps.
static std::unordered_map<int, TileProps> TsxParse(const std::string& tsx_path) {
    std::unordered_map<int, TileProps> result;

    std::string content;
    if (!ReadFile(tsx_path.c_str(), content)) return result;

    size_t pos = 0;
    while (true) {
        // Find next <tile id="N"> opening tag
        const size_t tile_tag = content.find("<tile id=\"", pos);
        if (tile_tag == std::string::npos) break;

        const size_t id_start = tile_tag + 10; // skip '<tile id="'
        const size_t id_quote = content.find('"', id_start);
        if (id_quote == std::string::npos) break;

        int tile_id = 0;
        try { tile_id = std::stoi(content.substr(id_start, id_quote - id_start)); }
        catch (...) { pos = tile_tag + 1; continue; }

        // Extract the block up to </tile>
        const size_t close_tag = content.find("</tile>", tile_tag);
        const size_t block_end = (close_tag == std::string::npos) ? content.size() : close_tag;
        const std::string block = content.substr(tile_tag, block_end - tile_tag);

        TileProps props;

        // --- solid property:  name="solid" ... value="true/false" ---
        const size_t solid_name = block.find("name=\"solid\"");
        if (solid_name != std::string::npos) {
            const size_t val_pos = block.find("value=\"", solid_name);
            if (val_pos != std::string::npos) {
                const size_t v = val_pos + 7;
                props.solid = (block.substr(v, 4) == "true");
            }
        }

        // --- type property:  name="type" ... value="spawn|platform|kill|end" ---
        const size_t type_name = block.find("name=\"type\"");
        if (type_name != std::string::npos) {
            const size_t val_pos = block.find("value=\"", type_name);
            if (val_pos != std::string::npos) {
                const size_t v = val_pos + 7;
                const size_t q = block.find('"', v);
                if (q != std::string::npos)
                    props.type = block.substr(v, q - v);
            }
        }

        result[tile_id] = props;
        pos = tile_tag + 1;
    }
    return result;
}

// Map a TMJ GID to a canonical World tile char.
//   gid == 0         → air ' '
//   otherwise        → look up tile_id = gid - firstgid in the TSX table
static char GidToChar(int gid, int firstgid,
                       const std::unordered_map<int, TileProps>& tsx) {
    if (gid == 0) return ' ';
    const int tile_id = gid - firstgid;
    const auto it = tsx.find(tile_id);
    if (it == tsx.end()) return ' ';
    const std::string& t = it->second.type;
    if (t == "platform") return '0';
    if (t == "kill")     return 'K';
    if (t == "end")      return 'E';
    if (t == "spawn")    return 'X';
    if (t == "checkpoint")  return 'C';
    if (t == "chunk_entry") return 'I';
    if (t == "chunk_exit")  return 'O';
    return ' ';
}

// Map a TMJ GID to a solid flag.
// NOTE: kill-type tiles are always non-solid regardless of the TSX "solid" property:
// the player must be able to enter them so the kill-detection logic in ServerSession
// can fire. Making them solid would cause the physics engine to push the player out
// before any overlap check runs, so they would never die.
static bool GidToSolid(int gid, int firstgid,
                        const std::unordered_map<int, TileProps>& tsx) {
    if (gid == 0) return false;
    const int tile_id = gid - firstgid;
    const auto it = tsx.find(tile_id);
    if (it == tsx.end()) return false;
    // Kill tiles are non-solid by design.
    if (it->second.type == "kill") return false;
    return it->second.solid;
}

// ============================================================================
// World public interface
// ============================================================================

bool World::LoadFromGrid(int w, int h, const std::vector<std::string>& rows) {
    if (w <= 0 || h <= 0) return false;
    if (static_cast<int>(rows.size()) < h) return false;

    rows_.clear();
    solid_grid_.clear();
    width_  = w;
    height_ = h;

    rows_.resize(height_);
    solid_grid_.resize(height_);
    for (int y = 0; y < height_; ++y) {
        rows_[y] = rows[y];
        if (static_cast<int>(rows_[y].size()) < width_)
            rows_[y].resize(width_, ' ');
        solid_grid_[y].resize(width_);
        for (int x = 0; x < width_; ++x)
            solid_grid_[y][x] = (rows_[y][x] == '0');
    }
    return true;
}

bool World::LoadFromFile(const char* path) {
    const std::string p(path);
    // Dispatch based on file extension
    if (p.size() >= 4 && p.substr(p.size() - 4) == ".tmj")
        return LoadTmj(path);
    return LoadTxt(path);
}

bool World::IsSolid(int tx, int ty) const {
    if (ty < 0 || ty >= height_) return false;
    if (tx < 0 || tx >= width_)  return false;
    if (ty >= static_cast<int>(solid_grid_.size())) return false;
    const auto& row = solid_grid_[ty];
    if (tx >= static_cast<int>(row.size())) return false;
    return row[tx];
}

char World::GetTile(int tx, int ty) const {
    if (ty < 0 || ty >= height_) return ' ';
    if (tx < 0 || tx >= width_)  return ' ';
    return rows_[ty][tx];
}

void World::StripCheckpoints() {
    for (int y = 0; y < height_; ++y)
        for (int x = 0; x < width_; ++x)
            if (rows_[y][x] == 'C') rows_[y][x] = ' ';
}

// ============================================================================
// Private loaders
// ============================================================================

bool World::LoadTxt(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    rows_.clear();
    solid_grid_.clear();
    width_ = height_ = 0;

    std::string line;
    while (std::getline(file, line)) {
        width_ = std::max(width_, static_cast<int>(line.size()));
        rows_.push_back(std::move(line));
    }

    // Pad all rows to uniform width
    for (auto& row : rows_) {
        if (static_cast<int>(row.size()) < width_)
            row.resize(width_, ' ');
    }

    height_ = static_cast<int>(rows_.size());

    // Build solid grid: only '0' is solid in legacy format
    solid_grid_.resize(height_);
    for (int y = 0; y < height_; ++y) {
        solid_grid_[y].resize(width_);
        for (int x = 0; x < width_; ++x)
            solid_grid_[y][x] = (rows_[y][x] == '0');
    }

    return height_ > 0;
}

bool World::LoadTmj(const char* path) {
    std::string json;
    if (!ReadFile(path, json)) return false;

    // --- Extract map dimensions ---
    int map_w = 0, map_h = 0;
    if (!JsonGetInt(json, "width",  map_w) ||
        !JsonGetInt(json, "height", map_h) ||
        map_w <= 0 || map_h <= 0) return false;

    // --- Extract firstgid and tileset source path ---
    int firstgid = 1;
    JsonGetInt(json, "firstgid", firstgid);   // falls back to 1 on error

    std::string tsx_src;
    if (!JsonGetString(json, "source", tsx_src)) return false;

    // Resolve TSX path relative to the TMJ file's directory
    const std::string tsx_path = DirOf(std::string(path)) + "/" + tsx_src;

    // --- Parse the TSX tile properties ---
    const auto tsx = TsxParse(tsx_path);

    // --- Extract tile data array ---
    std::vector<int> data;
    if (!TmjParseData(json, data)) return false;

    const int expected = map_w * map_h;
    if (static_cast<int>(data.size()) < expected) return false;

    // --- Build rows_ and solid_grid_ ---
    rows_.clear();
    solid_grid_.clear();
    width_  = map_w;
    height_ = map_h;

    rows_.resize(height_);
    solid_grid_.resize(height_);

    for (int y = 0; y < height_; ++y) {
        rows_[y].resize(width_);
        solid_grid_[y].resize(width_);
        for (int x = 0; x < width_; ++x) {
            const int gid = data[y * width_ + x];
            rows_[y][x]       = GidToChar (gid, firstgid, tsx);
            solid_grid_[y][x] = GidToSolid(gid, firstgid, tsx);
        }
    }

    return true;
}
