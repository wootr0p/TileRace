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

void Player::Update(float dt)
{
	vector2 direction = input->GetRawInput();
	state.velocity.x = direction.x * 200.0f;
	state.velocity.y = direction.y * 200.0f;
	HandleCollisions();
	UpdatePosition(dt);
}

void Player::Render(SDL_Renderer* renderer)
{
	rect.x = (float)state.position.x;
	rect.y = (float)state.position.y;
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(renderer, &rect);
}

void Player::HandleCollisions()
{
	state.isColliding = false;
	bool collision[3][3] = {
		{ false, false, false },
		{ false, false, false },
		{ false, false, false }
	};

	// Controllo le tile adiacenti al player se sono solide
	int j = 0;
	for (int y = (int)state.position.y - PLAYER_HEIGHT;
		y <= state.position.y + PLAYER_HEIGHT;
		y += PLAYER_HEIGHT)
	{
		int i = 0;
		for (int x = (int)state.position.x - PLAYER_WIDTH;
			x <= state.position.x + PLAYER_WIDTH;
			x += PLAYER_WIDTH)
		{
			bool isSolid = level->GetTile(x / TILE_SIZE, y / TILE_SIZE)->IsSolid();
			if (isSolid) state.isColliding = true;
			collision[i][j] = isSolid;
			i++;
		}
		j++;
	}

	// Se c'è una collisione, imposto la velocità a 0 sull'asse corrispondente
	if (collision[0][0] || collision[0][1] || collision[0][2] ||
		collision[2][0] || collision[2][1] || collision[2][2]) {
		state.velocity.x = 0.0f;
	}
	if (collision[0][0] || collision[1][0] || collision[2][0] ||
		collision[0][2] || collision[1][2] || collision[2][2]) {
		state.velocity.y = 0.0f;
	}
}

void Player::UpdatePosition(float dt)
{
	state.position.x += state.velocity.x * dt;
	state.position.y += state.velocity.y * dt;
}