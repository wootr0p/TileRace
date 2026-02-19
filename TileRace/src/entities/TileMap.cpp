#include "TileMap.h"

TileMap::TileMap() {
	tiles = new Tile*[MAP_WIDTH];

	for (int i = 0; i < MAP_WIDTH; i++) {
		tiles[i] = new Tile[MAP_HEIGHT];
		for (int j = 0; j < MAP_HEIGHT; j++) {
			tiles[i][j] = Tile(i * TILE_SIZE, j * TILE_SIZE, true);
		}
	}
}

TileMap::~TileMap() {
	for (int i = 0; i < MAP_WIDTH; i++) {
		delete[] tiles[i];
	}
	delete[] tiles;
}

void TileMap::Render(SDL_Renderer* renderer) {
	for (int i = 0; i < MAP_WIDTH; i++) {
		for (int j = 0; j < MAP_HEIGHT; j++) {
			tiles[i][j].Render(renderer);
		}
	}
}