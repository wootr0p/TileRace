#pragma once
#include <SDL3/SDL.h>
#include "../engine/Definitions.h"

const int TILE_SIZE = 32;

enum class TileType {
	Empty,		// spazio vuoto
	Platform,	// '0' - piattaforma solida
	End,		// 'E' - fine del livello
	Spawn		// 'X' - spawn giocatore
};

class Tile {

public:
	Tile();
	Tile(float x, float y, TileType type);
	~Tile();

	void Render(SDL_Renderer* renderer);
	TileType GetType() const { return type; }
	bool IsSolid() const { return solid; }

private:
	vector2 position;
	bool solid = false;
	TileType type;
	SDL_FRect rect;
	SDL_Color color;
};