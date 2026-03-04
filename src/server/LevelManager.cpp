// LevelManager.cpp — implementazione di caricamento livello e spawn.

#include "LevelManager.h"
#include "LevelValidator.h"
#include "Protocol.h"     // MAX_GENERATED_LEVELS
#include "SpawnFinder.h"  // FindCenterSpawn (src/common)
#include <cstdio>
#include <random>

bool LevelManager::Load(const char* path) {
    World tmp;
    if (!tmp.LoadFromFile(path)) return false;
    world_ = tmp;

    const SpawnPos sp = FindCenterSpawn(world_);
    spawn_x_ = sp.x;
    spawn_y_ = sp.y;
    return true;
}

bool LevelManager::Generate(int level_num, const ChunkStore& store, uint32_t seed, bool validate) {
    static constexpr int MAX_RETRIES = 10;

    GeneratorParams params;
    params.level_num    = level_num;
    // total_levels kept at default (8) to preserve the full difficulty curve
    // even though only MAX_GENERATED_LEVELS (5) levels are played per session
    params.seed         = seed;

    World last_good;   // keep last generated in case all validations fail
    bool  any_generated = false;

    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        // After the first attempt, randomise the seed.
        if (attempt > 0)
            params.seed = static_cast<uint32_t>(std::random_device{}());

        World tmp;
        if (!LevelGenerator::Generate(store, params, tmp))
            continue;

        any_generated = true;
        last_good = tmp;

        if (!validate || LevelValidator::Validate(tmp)) {
            world_ = tmp;
            const SpawnPos sp = FindCenterSpawn(world_);
            spawn_x_ = sp.x;
            spawn_y_ = sp.y;
            printf("[LevelManager] generated level %d  (attempt %d/%d)  spawn=(%.0f, %.0f)  size=%dx%d%s\n",
                   level_num, attempt + 1, MAX_RETRIES, spawn_x_, spawn_y_,
                   world_.GetWidth(), world_.GetHeight(),
                   validate ? "" : "  [validation skipped]");
            return true;
        }
        printf("[LevelManager] level %d FAILED validation (attempt %d/%d)\n",
               level_num, attempt + 1, MAX_RETRIES);
    }

    // Fallback: accept the last generated level even if not validated.
    if (any_generated) {
        printf("[LevelManager] WARNING: all %d attempts failed validation for level %d — using last\n",
               MAX_RETRIES, level_num);
        world_ = last_good;
        const SpawnPos sp = FindCenterSpawn(world_);
        spawn_x_ = sp.x;
        spawn_y_ = sp.y;
        return true;
    }
    return false;
}

std::string LevelManager::BuildPath(int num) {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "assets/levels/tilemaps/Level%02d.tmj", num);
    return buf;
}
