#include "TileMap.h"
#include <filesystem>

TileMap::TileMap(string level_name) {

	FileIO file = FileIO();
	file.Open("assets/levels/" + level_name + ".txt");
	
	string line;

	// Conto i caratteri della prima linea del file per sapere la larghezza del livello.
	line = file.ReadLine();
	if (line.size() > 0) {
		level_width = line.size();
	}

	// Scorro le restanti righe per sapere l'altezza del livello.
	level_height = 1;
	while (line != "") {
		line = file.ReadLine();
		if (line != "") level_height++;
	}

	// Dichiaro in memoria l'array bidimensionale di tile in base alla larghezza e altezza del livello.
	tiles = new Tile * [level_width];
	for (int i = 0; i < level_width; ++i) {
		tiles[i] = new Tile[level_height];
	}

	// Scorro il file di testo e inizializzo le tile in base ai caratteri letti.
	file.Rewind();
	int j = 0;
	while (j < level_height) {
		line = file.ReadLine();
		if (line == "") break;
		int i = 0;
		for (char c : line) {
			if (i >= level_width) break;
			
			TileType type = TileType::Empty;
			switch (c)
			{
				case '0':
					type = TileType::Platform;
					break;
				case 'E':
					type = TileType::End;
					break;
				case 'X':
					type = TileType::Spawn;
					player_spawn = { (float)i * TILE_SIZE, (float)j * TILE_SIZE };
					break;
				default:
					TileType type = TileType::Empty;
					break;
			}
			tiles[i][j] = Tile(i * TILE_SIZE, j * TILE_SIZE, type);
			i++;
		}
		// Se la riga è più corta, riempio il resto con tile vuote
		for (; i < level_width; ++i) {
			tiles[i][j] = Tile(i * TILE_SIZE, j * TILE_SIZE, TileType::Empty);
		}
		j++;
	}
}

TileMap::~TileMap() {
	if (tiles) {
		for (int i = 0; i < level_width; i++) {
			delete[] tiles[i];
		}
		delete[] tiles;
	}
}

void TileMap::Render(SDL_Renderer* renderer) {
	if (!tiles) return;
	for (int i = 0; i < level_width; i++) {
		for (int j = 0; j < level_height; j++) {
			tiles[i][j].Render(renderer);
		}
	}
}

Tile* TileMap::GetTile(int grid_x, int grid_y)
{
	if (grid_x < 0 || grid_x >= level_width || grid_y < 0 || grid_y >= level_height) {
		return nullptr; // Out of bounds
	}
	return &tiles[grid_x][grid_y];
}

Tile* TileMap::GetTileAt(float x, float y)
{
	// Converto le coordinate pixel in coordinate di griglia
	int grid_x = (int)(x / TILE_SIZE);
	int grid_y = (int)(y / TILE_SIZE);
	return GetTile(grid_x, grid_y);
}
