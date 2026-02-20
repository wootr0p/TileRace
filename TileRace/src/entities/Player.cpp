#include "Player.h"

Player::Player() :
	color{0, 255, 255, 255},
	state{0.0f, 0.0f, 0.0f},
	rect{ 0.0f, 0.0f, (float)PLAYER_WIDTH, (float)PLAYER_HEIGHT }
{
	rect.x = (float)state.x;
	rect.y = (float)state.y;
	rect.w = (float)PLAYER_WIDTH;
	rect.h = (float)PLAYER_HEIGHT;
}

Player::~Player()
{

}

void Player::Update(float dt, InputManager* input)
{

}

void Player::Render(SDL_Renderer* renderer)
{
	rect.x = (float)state.x;
	rect.y = (float)state.y;
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(renderer, &rect);
}


