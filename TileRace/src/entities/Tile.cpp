#include "Tile.h"

Tile::Tile() :
	position{ 0.0f, 0.0f },
	type(TileType::Empty),
	color{ 0, 0, 0, 0 },
	rect{ 0.0f, 0.0f, (float)TILE_SIZE, (float)TILE_SIZE },
	solid(false)
{

}

Tile::Tile(float x, float y, TileType type) :
	position{x,y},
	type(type),
	color{0, 0, 0, 0},
	rect{ x, y, (float)TILE_SIZE, (float)TILE_SIZE },
	solid(false)
{
	switch (type) {
		case TileType::Platform:
			color = SDL_Color{0, 255, 180, 255};
			solid = true;
			break;
		case TileType::End:
			color = SDL_Color{255, 0, 180, 180};
			solid = false;
			break;
		case TileType::Spawn:
			color = SDL_Color{255, 180, 0, 180};
			solid = false;
			break;
		case TileType::Empty:
			color = SDL_Color{0, 0, 0, 0};
			solid = false;
			break;
	}
}

Tile::~Tile() {
	
}

void Tile::Render(SDL_Renderer* renderer) {
	#ifndef _DEBUG
	if (!solid) return;
	#endif
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(renderer, &rect);
}