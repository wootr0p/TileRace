#pragma once
#include <SDL3/SDL.h>

const int TILE_SIZE = 32;

class Tile {

public:
	Tile();
	Tile(int x, int y, bool solid);
	~Tile();

	void Render(SDL_Renderer* renderer);

private:
	int x;
	int y;
	bool solid;
	SDL_FRect rect;

};