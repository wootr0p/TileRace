// ChunkStore.cpp — loads all .tmj chunks from a directory, parses and classifies them.

#include "ChunkStore.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

// ============================================================================
// Portable directory listing
// ============================================================================

static std::vector<std::string> ListTmjFiles(const std::string& dir) {
    std::vector<std::string> result;

#ifdef _WIN32
    // First, collect .tmj files in this directory
    {
        WIN32_FIND_DATAA fd;
        const std::string pattern = dir + "\\*.tmj";
        HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    result.push_back(dir + "/" + fd.cFileName);
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }
    }
    // Then recurse into subdirectories
    {
        WIN32_FIND_DATAA fd;
        const std::string pattern = dir + "\\*";
        HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    fd.cFileName[0] != '.') {
                    auto sub = ListTmjFiles(dir + "/" + fd.cFileName);
                    result.insert(result.end(), sub.begin(), sub.end());
                }
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }
    }
#else
    DIR* d = opendir(dir.c_str());
    if (!d) return result;
    struct dirent* ent;
    while ((ent = readdir(d)) != nullptr) {
        if (ent->d_name[0] == '.') continue;
        const std::string full = dir + "/" + ent->d_name;
        const std::string name = ent->d_name;
        if (name.size() >= 4 && name.substr(name.size() - 4) == ".tmj")
            result.push_back(full);
        // Recurse into subdirectories
        else if (ent->d_type == DT_DIR) {
            auto sub = ListTmjFiles(full);
            result.insert(result.end(), sub.begin(), sub.end());
        }
    }
    closedir(d);
#endif

    return result;
}

// ============================================================================
// Minimal JSON / TMJ helpers (duplicated from World.cpp to keep ChunkStore self-contained)
// ============================================================================

static bool ReadFileStr(const char* path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

static std::string DirOfPath(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? std::string(".") : path.substr(0, pos);
}

static bool JGetInt(const std::string& json, const std::string& key, int& out) {
    const std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return false;
    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return false;
    // Handle negative numbers too
    bool neg = false;
    if (json[pos] == '-') { neg = true; ++pos; }
    if (pos >= json.size() || !std::isdigit(static_cast<unsigned char>(json[pos]))) return false;
    try { out = std::stoi(json.substr(neg ? pos - 1 : pos)); return true; }
    catch (...) { return false; }
}

static bool JGetString(const std::string& json, const std::string& key, std::string& out) {
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
    for (size_t i = 0; i + 1 < out.size(); ) {
        if (out[i] == '\\' && out[i + 1] == '/') { out.erase(i, 1); }
        else ++i;
    }
    return true;
}

static bool ParseDataArray(const std::string& json, std::vector<int>& data) {
    const size_t data_key = json.find("\"data\"");
    if (data_key == std::string::npos) return false;
    const size_t arr_open = json.find('[', data_key);
    if (arr_open == std::string::npos) return false;
    const size_t arr_close = json.find(']', arr_open);
    if (arr_close == std::string::npos) return false;

    size_t cur = arr_open + 1;
    while (cur < arr_close) {
        while (cur < arr_close &&
               (json[cur] == ' ' || json[cur] == ',' ||
                json[cur] == '\n' || json[cur] == '\r' || json[cur] == '\t')) ++cur;
        if (cur >= arr_close) break;
        size_t num_end = cur;
        while (num_end < arr_close && std::isdigit(static_cast<unsigned char>(json[num_end]))) ++num_end;
        if (num_end > cur) {
            try { data.push_back(std::stoi(json.substr(cur, num_end - cur))); }
            catch (...) { data.push_back(0); }
            cur = num_end;
        } else { ++cur; }
    }
    return !data.empty();
}

// ============================================================================
// TSX parsing (same as World.cpp)
// ============================================================================

struct ChunkTileProps {
    std::string type;
    bool        solid = false;
};

static std::unordered_map<int, ChunkTileProps> ParseTsx(const std::string& tsx_path) {
    std::unordered_map<int, ChunkTileProps> result;
    std::string content;
    if (!ReadFileStr(tsx_path.c_str(), content)) return result;

    size_t pos = 0;
    while (true) {
        const size_t tile_tag = content.find("<tile id=\"", pos);
        if (tile_tag == std::string::npos) break;
        const size_t id_start = tile_tag + 10;
        const size_t id_quote = content.find('"', id_start);
        if (id_quote == std::string::npos) break;

        int tile_id = 0;
        try { tile_id = std::stoi(content.substr(id_start, id_quote - id_start)); }
        catch (...) { pos = tile_tag + 1; continue; }

        const size_t close_tag = content.find("</tile>", tile_tag);
        const size_t block_end = (close_tag == std::string::npos) ? content.size() : close_tag;
        const std::string block = content.substr(tile_tag, block_end - tile_tag);

        ChunkTileProps props;
        const size_t solid_name = block.find("name=\"solid\"");
        if (solid_name != std::string::npos) {
            const size_t val_pos = block.find("value=\"", solid_name);
            if (val_pos != std::string::npos) {
                const size_t v = val_pos + 7;
                props.solid = (block.substr(v, 4) == "true");
            }
        }
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

static char TileToChar(int gid, int firstgid,
                        const std::unordered_map<int, ChunkTileProps>& tsx) {
    if (gid == 0) return ' ';
    const int tile_id = gid - firstgid;
    const auto it = tsx.find(tile_id);
    if (it == tsx.end()) return ' ';
    const std::string& t = it->second.type;
    if (t == "platform")    return '0';
    if (t == "kill")        return 'K';
    if (t == "end")         return 'E';
    if (t == "spawn")       return 'X';
    if (t == "checkpoint")  return 'C';
    if (t == "chunk_entry") return 'I';
    if (t == "chunk_exit")  return 'O';
    return ' ';
}

static bool TileToSolid(int gid, int firstgid,
                          const std::unordered_map<int, ChunkTileProps>& tsx) {
    if (gid == 0) return false;
    const int tile_id = gid - firstgid;
    const auto it = tsx.find(tile_id);
    if (it == tsx.end()) return false;
    if (it->second.type == "kill") return false;
    return it->second.solid;
}

// ============================================================================
// Parse TMJ map properties: chunk_role, difficulty, weight
// ============================================================================

// Find Nth occurrence of "key" in a "properties" array context.
// This is a lightweight approach: search for property objects in the JSON.
static void ParseMapProperties(const std::string& json, Chunk& chunk) {
    // Default values
    chunk.role = "any";
    chunk.difficulty = 1;
    chunk.weight = 1;

    // Find the "properties" array
    const size_t props_key = json.find("\"properties\"");
    if (props_key == std::string::npos) return;
    const size_t arr_open = json.find('[', props_key);
    if (arr_open == std::string::npos) return;
    const size_t arr_close = json.find(']', arr_open);
    if (arr_close == std::string::npos) return;
    const std::string props_block = json.substr(arr_open, arr_close - arr_open + 1);

    // Parse each property object { "name":"...", "value":... }
    size_t pos = 0;
    while (true) {
        const size_t name_key = props_block.find("\"name\"", pos);
        if (name_key == std::string::npos) break;

        // Get the property name
        size_t p = name_key + 6;
        while (p < props_block.size() && (props_block[p] == ' ' || props_block[p] == ':' || props_block[p] == '\t')) ++p;
        if (p >= props_block.size() || props_block[p] != '"') { pos = name_key + 1; continue; }
        ++p;
        const size_t name_end = props_block.find('"', p);
        if (name_end == std::string::npos) break;
        const std::string prop_name = props_block.substr(p, name_end - p);

        // Find the "value" for this property (search after the name)
        const size_t val_key = props_block.find("\"value\"", name_end);
        if (val_key == std::string::npos) { pos = name_end; continue; }
        size_t v = val_key + 7;
        while (v < props_block.size() && (props_block[v] == ' ' || props_block[v] == ':' || props_block[v] == '\t')) ++v;

        if (prop_name == "chunk_role") {
            if (v < props_block.size() && props_block[v] == '"') {
                ++v;
                const size_t ve = props_block.find('"', v);
                if (ve != std::string::npos)
                    chunk.role = props_block.substr(v, ve - v);
            }
        } else if (prop_name == "difficulty") {
            try { chunk.difficulty = std::stoi(props_block.substr(v)); } catch (...) {}
        } else if (prop_name == "weight") {
            try { chunk.weight = std::stoi(props_block.substr(v)); } catch (...) {}
        }

        pos = val_key + 1;
    }
}

// ============================================================================
// ChunkStore implementation
// ============================================================================

bool ChunkStore::ParseChunk(const char* path, Chunk& out) {
    std::string json;
    if (!ReadFileStr(path, json)) return false;

    int map_w = 0, map_h = 0;
    if (!JGetInt(json, "width", map_w) || !JGetInt(json, "height", map_h) ||
        map_w <= 0 || map_h <= 0) return false;

    int firstgid = 1;
    JGetInt(json, "firstgid", firstgid);

    std::string tsx_src;
    if (!JGetString(json, "source", tsx_src)) return false;
    const std::string tsx_path = DirOfPath(std::string(path)) + "/" + tsx_src;
    const auto tsx = ParseTsx(tsx_path);

    std::vector<int> data;
    if (!ParseDataArray(json, data)) return false;
    if (static_cast<int>(data.size()) < map_w * map_h) return false;

    // Build the chunk grids
    out.width  = map_w;
    out.height = map_h;
    out.rows.resize(map_h);
    out.solid_grid.resize(map_h);
    out.entry_tx = out.entry_ty = -1;
    out.exit_tx  = out.exit_ty  = -1;
    out.has_spawn = false;
    out.has_end   = false;

    for (int y = 0; y < map_h; ++y) {
        out.rows[y].resize(map_w);
        out.solid_grid[y].resize(map_w);
        for (int x = 0; x < map_w; ++x) {
            const int gid = data[y * map_w + x];
            const char ch = TileToChar(gid, firstgid, tsx);
            out.rows[y][x]       = ch;
            out.solid_grid[y][x] = TileToSolid(gid, firstgid, tsx);

            if (ch == 'I') {
                out.entries.push_back({x, y});
                // First entry becomes the primary (backward compat)
                if (out.entry_tx < 0) { out.entry_tx = x; out.entry_ty = y; }
            }
            if (ch == 'O') {
                out.exits.push_back({x, y});
                // First exit becomes the primary (backward compat)
                if (out.exit_tx < 0) { out.exit_tx = x; out.exit_ty = y; }
            }
            if (ch == 'X') out.has_spawn = true;
            if (ch == 'E') out.has_end   = true;
            if (ch == 'C') out.has_checkpoint = true;
        }
    }

    // Parse map custom properties
    ParseMapProperties(json, out);

    return true;
}

void ChunkStore::Classify(Chunk chunk) {
    const bool has_entry = (chunk.entry_tx >= 0);
    const bool has_exit  = (chunk.exit_tx  >= 0);

    // Helper: also populate checkpoint/normal mid sub-pools.
    auto add_to_mid = [this](const Chunk& c) {
        if (c.has_checkpoint)
            mid_checkpoint_.push_back(c);
        else
            mid_normal_.push_back(c);
    };

    if (chunk.role == "start") {
        start_.push_back(std::move(chunk));
    } else if (chunk.role == "end") {
        end_.push_back(std::move(chunk));
    } else if (chunk.role == "mid") {
        add_to_mid(chunk);
        mid_.push_back(std::move(chunk));
    } else {
        // role == "any": auto-classify based on tile content
        // A chunk may be added to multiple pools.
        bool classified = false;
        if (chunk.has_spawn && has_exit) {
            start_.push_back(chunk);
            classified = true;
        }
        if (chunk.has_end && has_entry) {
            end_.push_back(chunk);
            classified = true;
        }
        if (has_entry && has_exit && !chunk.has_spawn && !chunk.has_end) {
            add_to_mid(chunk);
            mid_.push_back(chunk);
            classified = true;
        }
        if (!classified) {
            // Fallback: if it has both entry and exit, use as mid
            if (has_entry && has_exit) {
                add_to_mid(chunk);
                mid_.push_back(std::move(chunk));
            } else {
                printf("[ChunkStore] WARNING: chunk with role='any' couldn't be classified, skipped\n");
            }
        }
    }
}

bool ChunkStore::LoadFromDirectory(const char* dir) {
    start_.clear();
    mid_.clear();
    end_.clear();
    fork_start_.clear();
    fork_end_.clear();
    mid_checkpoint_.clear();
    mid_normal_.clear();

    const auto files = ListTmjFiles(dir);
    printf("[ChunkStore] scanning '%s': found %zu .tmj files\n", dir, files.size());

    for (const auto& path : files) {
        Chunk chunk;
        if (ParseChunk(path.c_str(), chunk)) {
            printf("[ChunkStore]   loaded '%s' (%dx%d) role='%s' entry=(%d,%d) exit=(%d,%d)"
                   " entries=%d exits=%d checkpoint=%s\n",
                   path.c_str(), chunk.width, chunk.height,
                   chunk.role.c_str(),
                   chunk.entry_tx, chunk.entry_ty,
                   chunk.exit_tx, chunk.exit_ty,
                   static_cast<int>(chunk.entries.size()),
                   static_cast<int>(chunk.exits.size()),
                   chunk.has_checkpoint ? "yes" : "no");
            Classify(std::move(chunk));
        } else {
            printf("[ChunkStore]   FAILED to parse '%s'\n", path.c_str());
        }
    }

    printf("[ChunkStore] pools: %zu start, %zu mid (%zu checkpoint, %zu normal), %zu end\n",
           start_.size(), mid_.size(), mid_checkpoint_.size(), mid_normal_.size(), end_.size());

    // Compute min/max difficulty among mid chunks.
    if (!mid_.empty()) {
        min_mid_diff_ = mid_[0].difficulty;
        max_mid_diff_ = mid_[0].difficulty;
        for (const auto& c : mid_) {
            if (c.difficulty < min_mid_diff_) min_mid_diff_ = c.difficulty;
            if (c.difficulty > max_mid_diff_) max_mid_diff_ = c.difficulty;
        }
        printf("[ChunkStore] mid difficulty range: %d .. %d\n",
               min_mid_diff_, max_mid_diff_);
    }

    // Detect fork chunks (multi-exit / multi-entry) across all pools.
    DetectForkChunks();

    return IsReady();
}

void ChunkStore::DetectForkChunks() {
    fork_start_.clear();
    fork_end_.clear();

    // Scan all loaded chunks for multi-exit (fork start) and multi-entry (fork end).
    // A chunk with >1 exits can serve as a fork start point.
    // A chunk with >1 entries can serve as a fork merge point.
    auto scan_pool = [this](const std::vector<Chunk>& pool) {
        for (const auto& chunk : pool) {
            if (chunk.exits.size() > 1)
                fork_start_.push_back(chunk);
            if (chunk.entries.size() > 1)
                fork_end_.push_back(chunk);
        }
    };

    scan_pool(start_);
    scan_pool(mid_);
    scan_pool(end_);

    if (!fork_start_.empty() || !fork_end_.empty()) {
        printf("[ChunkStore] fork chunks: %zu fork_start (multi-exit), %zu fork_end (multi-entry)\n",
               fork_start_.size(), fork_end_.size());
    } else {
        printf("[ChunkStore] no fork chunks found — branching disabled\n");
    }
}
