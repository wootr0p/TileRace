#pragma once
#include <SDL3/SDL.h>
#include "Tile.h"

const int MAP_WIDTH = 100;
const int MAP_HEIGHT = 100;

class TileMap {
public:
	TileMap();
	~TileMap();
	void Render(SDL_Renderer* renderer);
private:
	Tile** tiles;
};