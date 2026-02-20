#pragma once
#include <SDL3/SDL.h>
#include "Tile.h"
#include "../utility/FileIO.h"
#include "../engine/Definitions.h"
#include <string>

class TileMap {
public:
	TileMap(string level_name);
	~TileMap();
	void Render(SDL_Renderer* renderer);

	vector2 GetPlayerSpawn() { return player_spawn; }
	Tile* GetTile(int x, int y);
private:
	Tile** tiles = nullptr;
	int level_width = 0;
	int level_height = 0;
	vector2 player_spawn = { 0.0, 0.0f };
};