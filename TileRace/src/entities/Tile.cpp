#include "Tile.h"

Tile::Tile() :
	x(0),
	y(0),
	type(TileType::Empty)
{
	rect.x = 0.0f;
	rect.y = 0.0f;
	rect.w = rect.h = (float)TILE_SIZE;
}

Tile::Tile(int x, int y, TileType type) :
	x(x),
	y(y),
	type(type)
{
	rect.x = (float)x;
	rect.y = (float)y;
	rect.w = rect.h = (float)TILE_SIZE;
}

Tile::~Tile() {
	// Nulla da pulire per ora (membri automatici)
}

void Tile::Render(SDL_Renderer* renderer) {
	#ifndef _DEBUG
	if (type != TileType::Solid) return;
	#endif
	
	switch (type) {
		case TileType::Solid:
			SDL_SetRenderDrawColor(renderer, 0, 255, 180, 255);
			break;
	#ifdef _DEBUG
		case TileType::End:
			SDL_SetRenderDrawColor(renderer, 255, 0, 180, 180);
			break;
		case TileType::Spawn:
			SDL_SetRenderDrawColor(renderer, 255, 180, 0, 180);
			break;
		case TileType::Empty:
			return;// Non renderizzo le tile vuote.
	#endif
	}

	SDL_RenderFillRect(renderer, &rect);
}