// LevelGenerator.cpp — composes chunks into a playable level.
//
// Algorithm:
//   1. Compute target difficulty from level_num / total_levels
//   2. Pick 1 start chunk (random, weighted)
//   3. Pick N mid chunks filtered by difficulty band (N grows with level)
//   4. Pick 1 end chunk (random, weighted)
//   5. Optionally insert fork sections (if multi-exit/entry chunks exist):
//      - fork_start chunk splits the path into parallel branches
//      - each branch gets mid chunks
//      - fork_end chunk merges branches, OR each branch gets its own end
//      - 2-tile solid gap between branches
//   6. Stitch them by overlapping entry(N+1) on exit(N)
//   7. Normalise to (0,0) origin
//   8. Add 5-tile solid border + 5-tile empty margin
//   9. Strip I/O tiles (replace with air)
//  10. Load into World via LoadFromGrid

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

static int64_t GridKey(int x, int y) {
    return (static_cast<int64_t>(y) << 32) | static_cast<uint32_t>(x);
}

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
            grid[GridKey(wx, wy)] = { ch, chunk.solid_grid[cy][cx] };
            min_x = std::min(min_x, wx);
            min_y = std::min(min_y, wy);
            max_x = std::max(max_x, wx);
            max_y = std::max(max_y, wy);
        }
    }
}

// ============================================================================
// Bounding-box helper for a chunk sequence
// ============================================================================

struct BBox {
    int min_x = INT_MAX, min_y = INT_MAX;
    int max_x = INT_MIN, max_y = INT_MIN;

    bool Valid() const { return min_x <= max_x && min_y <= max_y; }
    int Width()  const { return max_x - min_x + 1; }
    int Height() const { return max_y - min_y + 1; }

    void Expand(int x, int y) {
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
    }
};

// Compute the bounding box of a sequence of chunks placed linearly
// starting from (start_wx, start_wy). Does not blit anything.
static BBox ComputeBranchBBox(const std::vector<const Chunk*>& chunks,
                               int start_wx, int start_wy) {
    BBox bb;
    int prev_exit_wx = start_wx;
    int prev_exit_wy = start_wy;

    for (const Chunk* c : chunks) {
        int off_x = 0, off_y = 0;
        if (c->entry_tx >= 0 && c->entry_ty >= 0) {
            off_x = prev_exit_wx - c->entry_tx;
            off_y = prev_exit_wy - c->entry_ty;
        } else {
            off_x = prev_exit_wx;
            off_y = prev_exit_wy + 1;
        }

        for (int cy = 0; cy < c->height; ++cy)
            for (int cx = 0; cx < c->width; ++cx)
                if (c->rows[cy][cx] != ' ')
                    bb.Expand(off_x + cx, off_y + cy);

        if (c->exit_tx >= 0) {
            prev_exit_wx = off_x + c->exit_tx;
            prev_exit_wy = off_y + c->exit_ty;
        }
    }
    return bb;
}

// Place a linear sequence of chunks onto the grid, starting by aligning
// the first chunk's entry to (start_wx + extra_off_x, start_wy).
// Returns the last exit world position.
static std::pair<int,int> PlaceLinearSequence(
    std::unordered_map<int64_t, TileCell>& grid,
    const std::vector<const Chunk*>& chunks,
    int start_wx, int start_wy,
    int extra_off_x,
    int& min_x, int& min_y, int& max_x, int& max_y)
{
    int prev_exit_wx = start_wx + extra_off_x;
    int prev_exit_wy = start_wy;

    for (const Chunk* c : chunks) {
        int off_x = 0, off_y = 0;
        if (c->entry_tx >= 0 && c->entry_ty >= 0) {
            off_x = prev_exit_wx - c->entry_tx;
            off_y = prev_exit_wy - c->entry_ty;
        } else {
            off_x = prev_exit_wx;
            off_y = prev_exit_wy + 1;
        }
        BlitChunk(grid, *c, off_x, off_y, min_x, min_y, max_x, max_y);

        if (c->exit_tx >= 0) {
            prev_exit_wx = off_x + c->exit_tx;
            prev_exit_wy = off_y + c->exit_ty;
        }
    }

    return {prev_exit_wx, prev_exit_wy};
}

// ============================================================================
// Difficulty curve helpers
// ============================================================================

float LevelGenerator::TargetDifficulty(int level_num, int total_levels) {
    if (total_levels <= 1) return 0.5f;
    return static_cast<float>(level_num - 1) / static_cast<float>(total_levels - 1);
}

int LevelGenerator::MidChunkCount(int level_num, int total_levels) {
    const float t = TargetDifficulty(level_num, total_levels);
    constexpr int MIN_MIDS =  4;
    constexpr int MAX_MIDS = 14;
    int n = MIN_MIDS + static_cast<int>(std::round(t * (MAX_MIDS - MIN_MIDS)));
    return std::clamp(n, MIN_MIDS, MAX_MIDS);
}

int LevelGenerator::MaxSimultaneousPaths(float t) {
    // Easy (t≈0): up to 5 simultaneous paths.
    // Hard (t≈1): max 2 simultaneous paths.
    const int max_p = 5 - static_cast<int>(std::round(t * 3.0f));
    return std::clamp(max_p, 2, 5);
}

int LevelGenerator::MaxArrivals(float t) {
    // Easy (t≈0): up to 4 arrivals.
    // Hard (t≈1): 1 or 2 arrivals (randomized at generation time).
    const int max_a = 4 - static_cast<int>(std::round(t * 2.0f));
    return std::clamp(max_a, 1, 4);
}

// ============================================================================
// Pick a fork-start chunk with at most max_exits exits
// ============================================================================

static int PickForkStart(const std::vector<Chunk>& pool, int max_exits,
                          std::mt19937& rng) {
    std::vector<int> eligible;
    int total_weight = 0;
    for (int i = 0; i < static_cast<int>(pool.size()); ++i) {
        if (pool[i].ExitCount() <= max_exits) {
            eligible.push_back(i);
            total_weight += std::max(1, pool[i].weight);
        }
    }
    if (eligible.empty()) return -1;

    std::uniform_int_distribution<int> dist(0, total_weight - 1);
    int roll = dist(rng);
    for (int idx : eligible) {
        roll -= std::max(1, pool[idx].weight);
        if (roll < 0) return idx;
    }
    return eligible.back();
}

// Pick a fork-end chunk with exactly n_entries entries (or closest match)
static int PickForkEnd(const std::vector<Chunk>& pool, int n_entries,
                        std::mt19937& rng) {
    // Prefer exact match
    std::vector<int> exact;
    int exact_weight = 0;
    for (int i = 0; i < static_cast<int>(pool.size()); ++i) {
        if (pool[i].EntryCount() == n_entries) {
            exact.push_back(i);
            exact_weight += std::max(1, pool[i].weight);
        }
    }
    if (!exact.empty()) {
        std::uniform_int_distribution<int> dist(0, exact_weight - 1);
        int roll = dist(rng);
        for (int idx : exact) {
            roll -= std::max(1, pool[idx].weight);
            if (roll < 0) return idx;
        }
        return exact.back();
    }

    // Fallback: any fork_end with >= n_entries
    std::vector<int> fallback;
    int fb_weight = 0;
    for (int i = 0; i < static_cast<int>(pool.size()); ++i) {
        if (pool[i].EntryCount() >= n_entries) {
            fallback.push_back(i);
            fb_weight += std::max(1, pool[i].weight);
        }
    }
    if (fallback.empty()) return -1;

    std::uniform_int_distribution<int> dist(0, fb_weight - 1);
    int roll = dist(rng);
    for (int idx : fallback) {
        roll -= std::max(1, pool[idx].weight);
        if (roll < 0) return idx;
    }
    return fallback.back();
}

// ============================================================================
// Minimum gap between parallel branches
// ============================================================================

static constexpr int BRANCH_GAP = 2;

// ============================================================================
// Build a linear chunk sequence and place it on the grid.
// This is the original algorithm (start + N mid + end), used when no
// fork chunks are available or as a fallback.
// ============================================================================

static void BuildAndPlaceLinear(
    std::unordered_map<int64_t, TileCell>& grid,
    const ChunkStore& store, int start_idx, int end_idx,
    int mid_count, int band_lo, int band_hi,
    std::mt19937& rng,
    int& min_x, int& min_y, int& max_x, int& max_y)
{
    std::vector<const Chunk*> sequence;
    sequence.push_back(&store.StartChunks()[start_idx]);
    for (int i = 0; i < mid_count; ++i)
        sequence.push_back(&store.MidChunks()[PickMidByDifficulty(
            store.MidChunks(), band_lo, band_hi, rng)]);
    sequence.push_back(&store.EndChunks()[end_idx]);

    // Stitch chunks via entry/exit alignment
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
}

// ============================================================================
// Build a branching level and place it on the grid.
// Returns true on success, false to signal the caller to fall back to linear.
// ============================================================================

static bool BuildAndPlaceBranching(
    std::unordered_map<int64_t, TileCell>& grid,
    const ChunkStore& store, int start_idx, int end_idx,
    int mid_count, int band_lo, int band_hi,
    int max_paths, int max_arrivals,
    std::mt19937& rng,
    int& min_x, int& min_y, int& max_x, int& max_y)
{
    // Distribute mid chunks: ~30% pre-fork, ~50% in branches, ~20% post-merge
    const int pre_fork_mids  = std::max(1, mid_count * 3 / 10);
    const int branch_mids    = std::max(2, mid_count * 5 / 10);
    const int post_merge_mids = std::max(0, mid_count - pre_fork_mids - branch_mids);

    // Pick fork_start chunk
    const int fork_start_idx = PickForkStart(store.ForkStartChunks(), max_paths, rng);
    if (fork_start_idx < 0) return false;

    const Chunk& fork_start = store.ForkStartChunks()[fork_start_idx];
    const int n_branches = std::min(fork_start.ExitCount(), max_paths);

    // Decide how many arrivals (end points) this level has
    int n_arrivals;
    {
        const int effective_max = std::min(max_arrivals, n_branches);
        if (effective_max <= 1)
            n_arrivals = 1;
        else {
            std::uniform_int_distribution<int> adist(1, effective_max);
            n_arrivals = adist(rng);
        }
    }

    // Decide if we merge branches back using a fork_end chunk
    const bool want_merge = (n_arrivals < n_branches) &&
                             !store.ForkEndChunks().empty();
    const Chunk* fork_end_ptr = nullptr;
    if (want_merge) {
        const int fe_idx = PickForkEnd(store.ForkEndChunks(), n_branches, rng);
        if (fe_idx >= 0)
            fork_end_ptr = &store.ForkEndChunks()[fe_idx];
    }

    printf("[LevelGenerator] FORK: %d branches, %d arrivals, merge=%s\n",
           n_branches, n_arrivals, fork_end_ptr ? "yes" : "no");

    // === Phase 1: Place start chunk + pre-fork mid chunks ===
    std::vector<const Chunk*> pre_fork;
    pre_fork.push_back(&store.StartChunks()[start_idx]);
    for (int i = 0; i < pre_fork_mids; ++i)
        pre_fork.push_back(&store.MidChunks()[PickMidByDifficulty(
            store.MidChunks(), band_lo, band_hi, rng)]);

    auto [trunk_exit_wx, trunk_exit_wy] = PlaceLinearSequence(
        grid, pre_fork, 0, 0, 0, min_x, min_y, max_x, max_y);

    // === Phase 2: Place fork_start chunk ===
    int fork_off_x = 0, fork_off_y = 0;
    if (fork_start.entry_tx >= 0) {
        fork_off_x = trunk_exit_wx - fork_start.entry_tx;
        fork_off_y = trunk_exit_wy - fork_start.entry_ty;
    } else {
        fork_off_x = trunk_exit_wx;
        fork_off_y = trunk_exit_wy + 1;
    }
    BlitChunk(grid, fork_start, fork_off_x, fork_off_y,
              min_x, min_y, max_x, max_y);

    // Collect exit world positions from the fork_start chunk
    std::vector<std::pair<int,int>> fork_exits;
    for (int b = 0; b < n_branches; ++b) {
        const auto& [ex, ey] = fork_start.exits[b];
        fork_exits.push_back({fork_off_x + ex, fork_off_y + ey});
    }

    // === Phase 3: Build branch chunk sequences ===
    const int mids_per_branch = std::max(1, branch_mids / n_branches);
    std::vector<std::vector<const Chunk*>> branch_chunks(n_branches);
    for (int b = 0; b < n_branches; ++b) {
        for (int j = 0; j < mids_per_branch; ++j)
            branch_chunks[b].push_back(&store.MidChunks()[
                PickMidByDifficulty(store.MidChunks(), band_lo, band_hi, rng)]);
    }

    // === Phase 4: Compute bounding boxes and offsets for gap enforcement ===
    std::vector<BBox> branch_bboxes(n_branches);
    for (int b = 0; b < n_branches; ++b)
        branch_bboxes[b] = ComputeBranchBBox(
            branch_chunks[b], fork_exits[b].first, fork_exits[b].second);

    // Sort branches left-to-right by their fork exit x-position
    std::vector<int> branch_order(n_branches);
    for (int b = 0; b < n_branches; ++b) branch_order[b] = b;
    std::sort(branch_order.begin(), branch_order.end(),
        [&](int a, int b_idx) {
            return fork_exits[a].first < fork_exits[b_idx].first;
        });

    // Compute horizontal offsets ensuring BRANCH_GAP tiles between branches
    std::vector<int> branch_x_offsets(n_branches, 0);
    for (int i = 1; i < n_branches; ++i) {
        const int prev_b = branch_order[i-1];
        const int curr_b = branch_order[i];
        const int prev_right = branch_bboxes[prev_b].max_x + branch_x_offsets[prev_b];
        const int curr_left  = branch_bboxes[curr_b].min_x;
        const int needed = (prev_right + BRANCH_GAP + 1) - curr_left;
        if (needed > 0)
            branch_x_offsets[curr_b] = needed;
    }

    // === Phase 5: Place branches with offsets ===
    std::vector<std::pair<int,int>> branch_exits(n_branches);
    for (int b = 0; b < n_branches; ++b) {
        auto [bx, by] = PlaceLinearSequence(
            grid, branch_chunks[b],
            fork_exits[b].first, fork_exits[b].second,
            branch_x_offsets[b],
            min_x, min_y, max_x, max_y);
        branch_exits[b] = {bx, by};
    }

    // === Phase 6: Merge or end branches ===
    if (fork_end_ptr && want_merge) {
        const Chunk& fe = *fork_end_ptr;

        // Find the topmost branch exit (smallest y = highest on screen)
        int merge_y = INT_MAX;
        for (int b = 0; b < n_branches; ++b)
            merge_y = std::min(merge_y, branch_exits[b].second);

        // Place fork_end, aligning its first entry to the first branch exit
        int fe_off_x = 0, fe_off_y = 0;
        if (fe.entry_tx >= 0) {
            fe_off_x = branch_exits[0].first - fe.entry_tx;
            fe_off_y = merge_y - fe.entry_ty;
        } else {
            fe_off_x = branch_exits[0].first;
            fe_off_y = merge_y + 1;
        }
        BlitChunk(grid, fe, fe_off_x, fe_off_y, min_x, min_y, max_x, max_y);

        // Continue from fork_end's exit with post-merge chunks + end
        const int merge_exit_wx = fe_off_x + fe.exit_tx;
        const int merge_exit_wy = fe_off_y + fe.exit_ty;

        std::vector<const Chunk*> post_merge;
        for (int i = 0; i < post_merge_mids; ++i)
            post_merge.push_back(&store.MidChunks()[PickMidByDifficulty(
                store.MidChunks(), band_lo, band_hi, rng)]);
        post_merge.push_back(&store.EndChunks()[end_idx]);

        PlaceLinearSequence(grid, post_merge,
            merge_exit_wx, merge_exit_wy, 0,
            min_x, min_y, max_x, max_y);
    } else {
        // No merge: each branch gets its own end chunk (up to n_arrivals)
        for (int b = 0; b < n_branches; ++b) {
            std::vector<const Chunk*> branch_end;
            if (b < n_arrivals) {
                const int be_idx = PickWeighted(store.EndChunks(), rng);
                branch_end.push_back(&store.EndChunks()[be_idx]);
            } else {
                // Dead-end branch: add some extra mid chunks, no 'E' tile
                for (int j = 0; j < post_merge_mids; ++j)
                    branch_end.push_back(&store.MidChunks()[
                        PickMidByDifficulty(store.MidChunks(), band_lo, band_hi, rng)]);
            }
            if (!branch_end.empty()) {
                PlaceLinearSequence(grid, branch_end,
                    branch_exits[b].first, branch_exits[b].second, 0,
                    min_x, min_y, max_x, max_y);
            }
        }
    }

    return true;
}

// ============================================================================
// Finalize grid → World
// ============================================================================

static bool FinalizeGrid(std::unordered_map<int64_t, TileCell>& grid,
                          int min_x, int min_y, int max_x, int max_y,
                          World& world) {
    // Strip entry/exit tiles (I, O → air)
    for (auto& [key, cell] : grid) {
        if (cell.ch == 'I' || cell.ch == 'O') {
            cell.ch    = ' ';
            cell.solid = false;
        }
    }

    // Build the final rectangular grid with 5-tile solid border + 5-tile margin
    const int border = 5;
    const int margin = 5;
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
            if (x < pad || x >= final_w - pad ||
                y < pad || y >= final_h - pad) {
                continue;
            }
            const int wx = min_x + (x - pad);
            const int wy = min_y + (y - pad);
            const auto it = grid.find(GridKey(wx, wy));
            if (it != grid.end())
                rows[y][x] = it->second.ch;
        }
    }

    // Kill strip at the very bottom (inside the border)
    for (int y = final_h - border - 1; y >= final_h - border - 2 && y >= border; --y)
        for (int x = border; x < final_w - border; ++x)
            rows[y][x] = 'K';

    printf("[LevelGenerator] final map: %d x %d tiles\n", final_w, final_h);

    return world.LoadFromGrid(final_w, final_h, rows);
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
    const int band_lo = std::max(d_min, static_cast<int>(std::floor(target_d - 0.5f)));
    const int band_hi = std::min(d_max, static_cast<int>(std::ceil(target_d + 0.5f)));

    // --- Branching parameters ---
    const int max_paths    = MaxSimultaneousPaths(t);
    const int max_arrivals = MaxArrivals(t);
    const bool can_fork    = store.HasForkChunks() && max_paths > 1;

    // --- Pick chunks ---
    const int start_idx = PickWeighted(store.StartChunks(), rng);
    const int end_idx   = PickWeighted(store.EndChunks(),   rng);
    const int mid_count = MidChunkCount(params.level_num, params.total_levels);

    printf("[LevelGenerator] level=%d/%d seed=%u  t=%.2f  diff_band=[%d,%d] (target=%.1f)  "
           "start=#%d  mids=%d  end=#%d  max_paths=%d max_arrivals=%d fork=%s\n",
           params.level_num, params.total_levels, seed, t,
           band_lo, band_hi, target_d,
           start_idx, mid_count, end_idx,
           max_paths, max_arrivals, can_fork ? "yes" : "no");

    // --- Sparse grid ---
    std::unordered_map<int64_t, TileCell> grid;
    int min_x = INT_MAX, min_y = INT_MAX;
    int max_x = INT_MIN, max_y = INT_MIN;

    if (can_fork) {
        bool ok = BuildAndPlaceBranching(
            grid, store, start_idx, end_idx,
            mid_count, band_lo, band_hi,
            max_paths, max_arrivals, rng,
            min_x, min_y, max_x, max_y);
        if (!ok) {
            printf("[LevelGenerator] branching failed, falling back to linear\n");
            grid.clear();
            min_x = INT_MAX; min_y = INT_MAX;
            max_x = INT_MIN; max_y = INT_MIN;
            BuildAndPlaceLinear(grid, store, start_idx, end_idx,
                                mid_count, band_lo, band_hi, rng,
                                min_x, min_y, max_x, max_y);
        }
    } else {
        BuildAndPlaceLinear(grid, store, start_idx, end_idx,
                            mid_count, band_lo, band_hi, rng,
                            min_x, min_y, max_x, max_y);
    }

    return FinalizeGrid(grid, min_x, min_y, max_x, max_y, world);
}
