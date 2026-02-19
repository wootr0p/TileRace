#pragma once
#include <SDL3/SDL.h>
#include "Tile.h"
#include "../utility/FileIO.h"
#include <string>

class TileMap {
public:
	TileMap(string level_name);
	~TileMap();
	void Render(SDL_Renderer* renderer);
private:
	Tile** tiles = nullptr;
	int level_width = 0;
	int level_height = 0;
};