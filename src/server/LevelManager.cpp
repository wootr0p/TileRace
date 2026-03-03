// LevelManager.cpp — implementazione di caricamento livello e spawn.

#include "LevelManager.h"
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

std::string LevelManager::BuildPath(int num) {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "assets/levels/tilemaps/Level%02d.tmj", num);
    return buf;
}
