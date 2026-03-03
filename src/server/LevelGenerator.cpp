// LevelGenerator.cpp — composes chunks into a playable level.
//
// Algorithm:
//   1. Compute target difficulty from level_num / total_levels
//   2. Pick 1 start chunk (random, weighted)
//   3. Pick N mid chunks filtered by difficulty band (N grows with level)
//   4. Pick 1 end chunk (random, weighted)
//   5. Stitch them by overlapping entry(N+1) on exit(N)
//   6. Normalise to (0,0) origin
//   7. Add 5-tile solid border + 5-tile empty margin
//   8. Strip I/O tiles (replace with air)
//   9. Load into World via LoadFromGrid

#include "LevelGenerator.h"
#include <algorithm>
#include <random>
#include <cstdio>
#include <cmath>
#include <climits>
#include <unordered_map>

// ============================================================================
// Weighted random pick (from a pre-filtered list of indices)
// ============================================================================

static int PickWeighted(const std::vector<Chunk>& pool, std::mt19937& rng) {
    if (pool.size() == 1) return 0;
    int total_weight = 0;
    for (const auto& c : pool) total_weight += std::max(1, c.weight);
    std::uniform_int_distribution<int> dist(0, total_weight - 1);
    int roll = dist(rng);
    for (int i = 0; i < static_cast<int>(pool.size()); ++i) {
        roll -= std::max(1, pool[i].weight);
        if (roll < 0) return i;
    }
    return static_cast<int>(pool.size()) - 1;
}

// Pick a random mid chunk whose difficulty is within [min_d, max_d].
// Falls back to the full pool if no chunk matches the filter.
static int PickMidByDifficulty(const std::vector<Chunk>& mids,
                                int min_d, int max_d, std::mt19937& rng) {
    // Build a filtered list of eligible indices
    std::vector<int> eligible;
    int total_weight = 0;
    for (int i = 0; i < static_cast<int>(mids.size()); ++i) {
        if (mids[i].difficulty >= min_d && mids[i].difficulty <= max_d) {
            eligible.push_back(i);
            total_weight += std::max(1, mids[i].weight);
        }
    }
    // Fallback: if nothing matches, use the entire pool
    if (eligible.empty()) return PickWeighted(mids, rng);

    // Weighted pick from eligible
    std::uniform_int_distribution<int> dist(0, total_weight - 1);
    int roll = dist(rng);
    for (int idx : eligible) {
        roll -= std::max(1, mids[idx].weight);
        if (roll < 0) return idx;
    }
    return eligible.back();
}

// ============================================================================
// Sparse grid for compositing
// ============================================================================

struct TileCell {
    char ch    = ' ';
    bool solid = false;
};

// Blit a chunk's non-air tiles onto the sparse grid at the given offset.
static void BlitChunk(std::unordered_map<int64_t, TileCell>& grid,
                       const Chunk& chunk, int off_x, int off_y,
                       int& min_x, int& min_y, int& max_x, int& max_y) {
    for (int cy = 0; cy < chunk.height; ++cy) {
        for (int cx = 0; cx < chunk.width; ++cx) {
            const char ch = chunk.rows[cy][cx];
            if (ch == ' ') continue; // don't overwrite with air
            const int wx = off_x + cx;
            const int wy = off_y + cy;
            const int64_t key = (static_cast<int64_t>(wy) << 32) | static_cast<uint32_t>(wx);
            grid[key] = { ch, chunk.solid_grid[cy][cx] };
            min_x = std::min(min_x, wx);
            min_y = std::min(min_y, wy);
            max_x = std::max(max_x, wx);
            max_y = std::max(max_y, wy);
        }
    }
}

// ============================================================================
// Difficulty curve helpers
// ============================================================================

float LevelGenerator::TargetDifficulty(int level_num, int total_levels) {
    // Returns a value in [0, 1] representing progression through the session.
    // level_num is 1-based, total_levels >= 1.
    if (total_levels <= 1) return 0.5f;
    return static_cast<float>(level_num - 1) / static_cast<float>(total_levels - 1);
}

int LevelGenerator::MidChunkCount(int level_num, int total_levels) {
    // Grows linearly: first level → 4 chunks, last level → 14 chunks.
    const float t = TargetDifficulty(level_num, total_levels);
    constexpr int MIN_MIDS =  4;
    constexpr int MAX_MIDS = 14;
    int n = MIN_MIDS + static_cast<int>(std::round(t * (MAX_MIDS - MIN_MIDS)));
    return std::clamp(n, MIN_MIDS, MAX_MIDS);
}

// ============================================================================
// LevelGenerator::Generate
// ============================================================================

bool LevelGenerator::Generate(const ChunkStore& store, const GeneratorParams& params,
                               World& world) {
    if (!store.IsReady()) {
        printf("[LevelGenerator] ERROR: ChunkStore not ready\n");
        return false;
    }

    // Seed RNG
    const uint32_t seed = (params.seed != 0)
        ? params.seed
        : static_cast<uint32_t>(std::random_device{}());
    std::mt19937 rng(seed);

    // --- Difficulty band for this level ---
    const float t = TargetDifficulty(params.level_num, params.total_levels);
    const int d_min = store.MinMidDifficulty();
    const int d_max = store.MaxMidDifficulty();
    const float target_d = d_min + t * (d_max - d_min);
    // Allow chunks within ±1 of target, clamped to actual range
    const int band_lo = std::max(d_min, static_cast<int>(std::floor(target_d - 0.5f)));
    const int band_hi = std::min(d_max, static_cast<int>(std::ceil(target_d + 0.5f)));

    // --- Pick chunks ---
    const int start_idx = PickWeighted(store.StartChunks(), rng);
    const int end_idx   = PickWeighted(store.EndChunks(),   rng);
    const int mid_count = MidChunkCount(params.level_num, params.total_levels);

    std::vector<int> mid_indices;
    mid_indices.reserve(mid_count);
    for (int i = 0; i < mid_count; ++i)
        mid_indices.push_back(PickMidByDifficulty(store.MidChunks(), band_lo, band_hi, rng));

    printf("[LevelGenerator] level=%d/%d seed=%u  t=%.2f  diff_band=[%d,%d] (target=%.1f)  "
           "start=#%d  mids=%d  end=#%d\n",
           params.level_num, params.total_levels, seed, t,
           band_lo, band_hi, target_d,
           start_idx, mid_count, end_idx);

    // --- Build ordered chunk list ---
    std::vector<const Chunk*> sequence;
    sequence.push_back(&store.StartChunks()[start_idx]);
    for (int idx : mid_indices)
        sequence.push_back(&store.MidChunks()[idx]);
    sequence.push_back(&store.EndChunks()[end_idx]);

    // --- Stitch chunks via entry/exit alignment ---
    std::unordered_map<int64_t, TileCell> grid;
    grid.reserve(sequence.size() * 33 * 23);

    int min_x = INT_MAX, min_y = INT_MAX;
    int max_x = INT_MIN, max_y = INT_MIN;

    int prev_exit_wx = 0, prev_exit_wy = 0;
    {
        const Chunk& first = *sequence[0];
        BlitChunk(grid, first, 0, 0, min_x, min_y, max_x, max_y);
        if (first.exit_tx >= 0) {
            prev_exit_wx = first.exit_tx;
            prev_exit_wy = first.exit_ty;
        }
    }

    for (size_t i = 1; i < sequence.size(); ++i) {
        const Chunk& c = *sequence[i];
        int off_x = 0, off_y = 0;
        if (c.entry_tx >= 0 && c.entry_ty >= 0) {
            off_x = prev_exit_wx - c.entry_tx;
            off_y = prev_exit_wy - c.entry_ty;
        } else {
            off_x = prev_exit_wx;
            off_y = prev_exit_wy + 1;
        }
        BlitChunk(grid, c, off_x, off_y, min_x, min_y, max_x, max_y);

        if (c.exit_tx >= 0) {
            prev_exit_wx = off_x + c.exit_tx;
            prev_exit_wy = off_y + c.exit_ty;
        }
    }

    // --- Strip entry/exit tiles (I, O → air) ---
    for (auto& [key, cell] : grid) {
        if (cell.ch == 'I' || cell.ch == 'O') {
            cell.ch    = ' ';
            cell.solid = false;
        }
    }

    // --- Build the final rectangular grid with 5-tile solid border + 5-tile margin ---
    const int border = 5;   // solid wall thickness
    const int margin = 5;   // empty gap between wall and chunk content
    const int pad    = border + margin;
    const int content_w = max_x - min_x + 1;
    const int content_h = max_y - min_y + 1;
    const int final_w = content_w + 2 * pad;
    const int final_h = content_h + 2 * pad;

    std::vector<std::string> rows(final_h);
    for (int y = 0; y < final_h; ++y) {
        rows[y].resize(final_w, ' ');
        for (int x = 0; x < final_w; ++x) {
            if (x < border || x >= final_w - border ||
                y < border || y >= final_h - border) {
                rows[y][x] = '0';
                continue;
            }
            // Margin zone → leave as air (default ' ')
            if (x < pad || x >= final_w - pad ||
                y < pad || y >= final_h - pad) {
                continue;
            }
            const int wx = min_x + (x - pad);
            const int wy = min_y + (y - pad);
            const int64_t key = (static_cast<int64_t>(wy) << 32) | static_cast<uint32_t>(wx);
            const auto it = grid.find(key);
            if (it != grid.end())
                rows[y][x] = it->second.ch;
        }
    }

    printf("[LevelGenerator] final map: %d x %d tiles\n", final_w, final_h);

    // --- Load into World ---
    return world.LoadFromGrid(final_w, final_h, rows);
}
