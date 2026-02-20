#include "Tile.h"

Tile::Tile() :
	x(0),
	y(0),
	type(TileType::Empty),
	color{ 0, 0, 0, 0 }
{
	rect.x = 0.0f;
	rect.y = 0.0f;
	rect.w = rect.h = (float)TILE_SIZE;
}

Tile::Tile(int x, int y, TileType type) :
	x(x),
	y(y),
	type(type),
	color{0, 0, 0, 0}
{
	rect.x = (float)x;
	rect.y = (float)y;
	rect.w = rect.h = (float)TILE_SIZE;

	switch (type) {
		case TileType::Solid:
			color = SDL_Color{0, 255, 180, 255};
			break;
		case TileType::End:
			color = SDL_Color{255, 0, 180, 180};
			break;
		case TileType::Spawn:
			color = SDL_Color{255, 180, 0, 180};
			break;
		case TileType::Empty:
			color = SDL_Color{0, 0, 0, 0};
			break;
	}
}

Tile::~Tile() {
	
}

void Tile::Render(SDL_Renderer* renderer) {
	#ifndef _DEBUG
	if (type != TileType::Solid) return;
	#endif
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(renderer, &rect);
}