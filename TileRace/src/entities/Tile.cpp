#include "Tile.h"

Tile::Tile() :
	x(0),
	y(0),
	solid(true)
{
	rect.x = 0.0f;
	rect.y = 0.0f;
	rect.w = rect.h = (float)TILE_SIZE;
}

Tile::Tile(int x, int y, bool solid) :
	x(x),
	y(y),
	solid(solid)
{
	rect.x = (float)x;
	rect.y = (float)y;
	rect.w = rect.h = (float)TILE_SIZE;
}

Tile::~Tile() {
	// Nulla da pulire per ora (membri automatici)
}

void Tile::Render(SDL_Renderer* renderer) {
	SDL_SetRenderDrawColor(renderer, 0, 255, 180, 255);
	SDL_RenderFillRect(renderer, &rect);
}