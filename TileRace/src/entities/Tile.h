#pragma once
#include <SDL3/SDL.h>

const int TILE_SIZE = 32;

enum class TileType {
	Empty,	// spazio vuoto
	Solid,	// '0' - tile solida
	End,	// 'E' - fine del livello
	Spawn	// 'X' - spawn giocatore
};

class Tile {

public:
	Tile();
	Tile(int x, int y, TileType type);
	~Tile();

	void Render(SDL_Renderer* renderer);
	TileType GetType() const { return type; }

private:
	int x;
	int y;
	TileType type;
	SDL_FRect rect;
	SDL_Color color;
};