// LevelManager.cpp — implementazione di caricamento livello e spawn.

#include "LevelManager.h"
#include "Protocol.h"     // MAX_GENERATED_LEVELS
#include "SpawnFinder.h"  // FindCenterSpawn (src/common)
#include <cstdio>

bool LevelManager::Load(const char* path) {
    World tmp;
    if (!tmp.LoadFromFile(path)) return false;
    world_ = tmp;

    const SpawnPos sp = FindCenterSpawn(world_);
    spawn_x_ = sp.x;
    spawn_y_ = sp.y;
    return true;
}

bool LevelManager::Generate(int level_num, const ChunkStore& store, uint32_t seed) {
    GeneratorParams params;
    params.level_num    = level_num;
    params.total_levels = MAX_GENERATED_LEVELS;
    params.seed         = seed;

    World tmp;
    if (!LevelGenerator::Generate(store, params, tmp)) return false;
    world_ = tmp;

    const SpawnPos sp = FindCenterSpawn(world_);
    spawn_x_ = sp.x;
    spawn_y_ = sp.y;
    printf("[LevelManager] generated level %d  spawn=(%.0f, %.0f)  size=%dx%d\n",
           level_num, spawn_x_, spawn_y_, world_.GetWidth(), world_.GetHeight());
    return true;
}

std::string LevelManager::BuildPath(int num) {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "assets/levels/tilemaps/Level%02d.tmj", num);
    return buf;
}
