#include "Player.h"

Player::Player(InputManager* input, vector2 position, TileMap* level) :
	color{0, 255, 255, 255},
	state{ position.x, position.y, 0.0f},
	rect{ position.x, position.y, (float)PLAYER_WIDTH, (float)PLAYER_HEIGHT },
	input(input),
	level(level)
{

}

Player::~Player()
{

}

void Player::Update(float dt) {
	vector2 direction = input->GetRawInput();

	// Muovo il player
	state.velocity.x = direction.x * PLAYER_SPEED;
	state.velocity.y = direction.y * PLAYER_SPEED;
	state.position.x += state.velocity.x * dt;
	state.position.y += state.velocity.y * dt;

	// Risolvo le collisioni
    ResolveCollisions();
}

void Player::Render(SDL_Renderer* renderer)
{
	rect.x = state.position.x;
	rect.y = state.position.y;
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(renderer, &rect);
}

void Player::ResolveCollisions() {
    
}