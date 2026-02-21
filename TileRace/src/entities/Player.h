#pragma once
#include <SDL3/SDL.h>
#include "..\engine\Definitions.h"
#include "..\engine\InputManager.h"
#include "TileMap.h"
#include "..\engine\InputManager.h"


static const int PLAYER_WIDTH = 32;
static const int PLAYER_HEIGHT = 32;
static const float PLAYER_SPEED = 200.0f;

struct PlayerState {
	vector2 position;
	vector2 velocity;
	bool isColliding = false;
};

class Player
{
public:
	Player(InputManager* input, vector2 position, TileMap* level);
	~Player();

	void Update(float dt);
	void Render(SDL_Renderer* renderer);
	bool IsColliding() { return state.isColliding; }
	
private:
	SDL_Color color;
	PlayerState state;
	SDL_FRect rect;
	TileMap* level;
	InputManager* input;

	void ResolveCollisionsX();
	void ResolveCollisionsY();
};
