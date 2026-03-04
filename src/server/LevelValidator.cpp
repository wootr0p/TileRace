// LevelValidator.cpp — AI agent that BFS-explores a level using real physics.
//
// From each reachable ground tile, the agent tries a set of "macro-actions"
// (jump, dash, walk, wall-jump chains, dash-jump combos) by running
// Player::Simulate for up to 3 seconds of game time per action.
// The agent automatically wall-jumps whenever it contacts a wall mid-air.
// A level is valid if the agent can reach any 'E' tile from the spawn.

#include "LevelValidator.h"
#include "Player.h"
#include "SpawnFinder.h"
#include "Physics.h"
#include <unordered_set>
#include <queue>
#include <vector>
#include <cstdio>
#include <cmath>

// ============================================================================
// Action definitions
// ============================================================================

struct ActionDef {
    float move_dir;           // -1, 0, +1  horizontal input
    bool  initial_jump;       // BTN_JUMP_PRESS on tick 0
    bool  initial_dash;       // BTN_DASH on tick 0
    float dash_dx, dash_dy;   // dash direction (normalised)
    bool  jump_after_dash;    // press jump when dash-jump window opens on ground
};

static constexpr float D  = 0.707107f; // √2/2

static const ActionDef ACTIONS[] = {
    // Walk
    {+1.f, false, false, 0,0, false},                 //  0  walk right
    {-1.f, false, false, 0,0, false},                 //  1  walk left
    // Ground jump
    {+1.f, true,  false, 0,0, false},                 //  2  jump right
    {-1.f, true,  false, 0,0, false},                 //  3  jump left
    { 0.f, true,  false, 0,0, false},                 //  4  jump up
    // Dash (directional)
    { 0.f, false, true,  0.f, -1.f, false},           //  5  dash up
    {+1.f, false, true,  +D,  -D,   false},           //  6  dash up-right
    {-1.f, false, true,  -D,  -D,   false},           //  7  dash up-left
    {+1.f, false, true, +1.f, 0.f,  false},           //  8  dash right
    {-1.f, false, true, -1.f, 0.f,  false},           //  9  dash left
    // Dash up → dash-jump (fires on landing or if still on ground)
    {+1.f, false, true,  0.f, -1.f, true},            // 10  dash up → djump right
    {-1.f, false, true,  0.f, -1.f, true},            // 11  dash up → djump left
    { 0.f, false, true,  0.f, -1.f, true},            // 12  dash up → djump up
    // Dash diagonal → dash-jump
    {+1.f, false, true,  +D,  -D,   true},            // 13  dash UR → djump right
    {-1.f, false, true,  -D,  -D,   true},            // 14  dash UL → djump left
    // Horizontal dash → dash-jump (most reliable: player stays near ground)
    {+1.f, false, true, +1.f, 0.f,  true},            // 15  dash right → djump right
    {-1.f, false, true, -1.f, 0.f,  true},            // 16  dash left  → djump left
};

static constexpr int NUM_ACTIONS = static_cast<int>(sizeof(ACTIONS) / sizeof(ACTIONS[0]));

// ============================================================================
// Input generation — reactive per-tick
// ============================================================================

static InputFrame MakeInput(const ActionDef& act, int tick, const PlayerState& ps) {
    InputFrame f{};
    f.tick = static_cast<uint32_t>(tick);
    f.move_x = act.move_dir;

    if (act.move_dir > 0.f) f.buttons |= BTN_RIGHT;
    if (act.move_dir < 0.f) f.buttons |= BTN_LEFT;

    // --- Auto wall-jump: whenever on a wall mid-air, press jump away ---
    if (!ps.on_ground && ps.dash_active_ticks == 0 &&
        (ps.on_wall_left || ps.on_wall_right)) {
        f.buttons |= BTN_JUMP_PRESS | BTN_JUMP;
        if (ps.on_wall_left) {
            f.move_x = 1.f;
            f.buttons = static_cast<uint16_t>((f.buttons | BTN_RIGHT | BTN_JUMP_PRESS | BTN_JUMP) & ~BTN_LEFT);
        } else {
            f.move_x = -1.f;
            f.buttons = static_cast<uint16_t>((f.buttons | BTN_LEFT | BTN_JUMP_PRESS | BTN_JUMP) & ~BTN_RIGHT);
        }
        return f;
    }

    // --- Tick 0: initial action ---
    if (tick == 0) {
        if (act.initial_jump)
            f.buttons |= BTN_JUMP_PRESS;
        if (act.initial_dash) {
            f.buttons |= BTN_DASH;
            f.dash_dx = act.dash_dx;
            f.dash_dy = act.dash_dy;
        }
    }

    // Hold BTN_JUMP while rising (full-height jump)
    if (ps.vel_y < 0.f)
        f.buttons |= BTN_JUMP;

    // After-dash: trigger dash-jump when the window is open and we're on ground
    if (act.jump_after_dash && ps.dash_active_ticks == 0 &&
        ps.dash_jump_ticks > 0 &&
        (ps.on_ground || ps.coyote_ticks > 0)) {
        f.buttons |= BTN_JUMP_PRESS | BTN_JUMP;
    }

    // Maintain dash steering direction
    if (ps.dash_active_ticks > 0) {
        f.dash_dx = act.dash_dx;
        f.dash_dy = act.dash_dy;
    }

    return f;
}

// ============================================================================
// Tile-overlap helpers
// ============================================================================

static bool OverlapsTileType(float px, float py, const World& world, char type) {
    const int tx0 = static_cast<int>(px)                    / TILE_SIZE;
    const int ty0 = static_cast<int>(py)                    / TILE_SIZE;
    const int tx1 = static_cast<int>(px + TILE_SIZE - 1.f)  / TILE_SIZE;
    const int ty1 = static_cast<int>(py + TILE_SIZE - 1.f)  / TILE_SIZE;
    for (int ty = ty0; ty <= ty1; ++ty)
        for (int tx = tx0; tx <= tx1; ++tx)
            if (world.GetTile(tx, ty) == type) return true;
    return false;
}

// ============================================================================
// Single-action simulation
// ============================================================================

static constexpr int   MAX_SIM_TICKS = static_cast<int>(3.0f / FIXED_DT);  // 3 s of simulated time (scales with FIXED_DT)
static constexpr int   MAX_BFS_NODES = 50000;

struct SimResult {
    bool reached_end = false;
    std::vector<std::pair<int, int>> ground_tiles;
};

static SimResult SimulateAction(const ActionDef& act, int start_tx, int start_ty,
                                 const World& world) {
    SimResult result;

    Player player;
    PlayerState ps{};
    ps.x          = static_cast<float>(start_tx * TILE_SIZE);
    ps.y          = static_cast<float>(start_ty * TILE_SIZE);
    ps.on_ground  = true;
    ps.dash_ready = true;
    player.SetState(ps);

    bool was_airborne = false;
    int  ground_ticks = 0;      // consecutive ground ticks after becoming airborne
    float last_x = ps.x, last_y = ps.y;
    int stale_ticks = 0;

    std::unordered_set<int64_t> seen_tiles;

    for (int tick = 0; tick < MAX_SIM_TICKS; ++tick) {
        const PlayerState& cur = player.GetState();

        // --- Check for 'E' tile ---
        if (OverlapsTileType(cur.x, cur.y, world, 'E')) {
            result.reached_end = true;
            return result;
        }

        // --- Kill tile → abort this trajectory ---
        if (OverlapsTileType(cur.x, cur.y, world, 'K'))
            break;

        // --- Out of bounds → abort ---
        if (cur.x < 0.f || cur.y < 0.f ||
            cur.x > world.GetWidth()  * TILE_SIZE ||
            cur.y > world.GetHeight() * TILE_SIZE)
            break;

        // --- Generate input and simulate one tick ---
        const InputFrame input = MakeInput(act, tick, cur);
        player.Simulate(input, world);

        const PlayerState& after = player.GetState();

        // --- Record ground tiles ---
        if (after.on_ground) {
            int gtx = static_cast<int>(after.x) / TILE_SIZE;
            int gty = static_cast<int>(after.y) / TILE_SIZE;
            if (gtx != start_tx || gty != start_ty) {
                int64_t key = (static_cast<int64_t>(gty) << 32) |
                               static_cast<uint32_t>(gtx);
                if (seen_tiles.find(key) == seen_tiles.end()) {
                    seen_tiles.insert(key);
                    result.ground_tiles.push_back({gtx, gty});
                }
            }
        }

        // --- Termination heuristics ---
        if (!after.on_ground) was_airborne = true;

        if (was_airborne && after.on_ground) ground_ticks++;
        else ground_ticks = 0;

        // Landed after flight: give 5 extra ticks to slide, then stop
        if (was_airborne && ground_ticks >= 5) break;

        // Stuck detection (not moving)
        if (std::fabs(after.x - last_x) < 0.1f && std::fabs(after.y - last_y) < 0.1f)
            stale_ticks++;
        else
            stale_ticks = 0;
        last_x = after.x;
        last_y = after.y;
        if (stale_ticks > 15) break;
    }

    // Check end tile one last time after loop
    const PlayerState& final_ps = player.GetState();
    if (OverlapsTileType(final_ps.x, final_ps.y, world, 'E'))
        result.reached_end = true;

    return result;
}

// ============================================================================
// LevelValidator::Validate — BFS over ground tile positions
// ============================================================================

bool LevelValidator::Validate(const World& world) {
    // --- Find spawn ---
    const SpawnPos spawn = FindCenterSpawn(world);
    const int spawn_tx = static_cast<int>(spawn.x) / TILE_SIZE;
    const int spawn_ty = static_cast<int>(spawn.y) / TILE_SIZE;
    if (spawn.x == 0.f && spawn.y == 0.f) {
        printf("[LevelValidator] ERROR: no spawn found\n");
        return false;
    }

    // --- Verify at least one 'E' tile exists ---
    bool has_end = false;
    for (int ty = 0; ty < world.GetHeight() && !has_end; ++ty)
        for (int tx = 0; tx < world.GetWidth() && !has_end; ++tx)
            if (world.GetTile(tx, ty) == 'E') has_end = true;
    if (!has_end) {
        printf("[LevelValidator] ERROR: no 'E' tile found\n");
        return false;
    }

    // --- BFS ---
    auto key = [](int tx, int ty) -> int64_t {
        return (static_cast<int64_t>(ty) << 32) | static_cast<uint32_t>(tx);
    };

    std::unordered_set<int64_t> visited;
    std::queue<std::pair<int, int>> frontier;

    visited.insert(key(spawn_tx, spawn_ty));
    frontier.push({spawn_tx, spawn_ty});

    int expansions = 0;

    while (!frontier.empty() && expansions < MAX_BFS_NODES) {
        auto [tx, ty] = frontier.front();
        frontier.pop();
        ++expansions;

        for (int ai = 0; ai < NUM_ACTIONS; ++ai) {
            SimResult result = SimulateAction(ACTIONS[ai], tx, ty, world);

            if (result.reached_end) {
                printf("[LevelValidator] VALID — reached 'E' after %d BFS expansions\n",
                       expansions);
                return true;
            }

            for (auto [gtx, gty] : result.ground_tiles) {
                int64_t k = key(gtx, gty);
                if (visited.find(k) == visited.end()) {
                    visited.insert(k);
                    frontier.push({gtx, gty});
                }
            }
        }
    }

    printf("[LevelValidator] INVALID — exhausted %d BFS nodes, %zu tiles visited\n",
           expansions, visited.size());
    return false;
}
