#pragma once
#include <SDL3/SDL.h>
#include "..\engine\InputManager.h"

static const int PLAYER_WIDTH = 32;
static const int PLAYER_HEIGHT = 32;

struct PlayerState {
	float x;
	float y;
	float speed;
};

class Player
{
public:
	Player();
	~Player();

	void Update(float dt, InputManager* input);
	void Render(SDL_Renderer* renderer);
	
private:
	SDL_Color color;
	PlayerState state;
	SDL_FRect rect;
};
