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

	// --- ASSE X ---
	state.velocity.x = direction.x * PLAYER_SPEED;
	state.position.x += state.velocity.x * dt;
	ResolveCollisionsX();

	// --- ASSE Y ---
	state.velocity.y = direction.y * PLAYER_SPEED;
	state.position.y += state.velocity.y * dt;
	ResolveCollisionsY();
}

void Player::Render(SDL_Renderer* renderer)
{
	rect.x = state.position.x;
	rect.y = state.position.y;
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(renderer, &rect);
}

void Player::ResolveCollisionsX() {
    // Calcoliamo i limiti del player (AABB)
    float left = state.position.x;
    float right = state.position.x + PLAYER_WIDTH;
    float top = state.position.y;
    float bottom = state.position.y + PLAYER_HEIGHT;

    // Troviamo il range di tile da controllare (ottimizzazione)
    int startTileX = (int)(left / TILE_SIZE);
    int endTileX = (int)(right / TILE_SIZE);
    int startTileY = (int)(top / TILE_SIZE);
    int endTileY = (int)(bottom / TILE_SIZE);

    for (int ty = startTileY; ty <= endTileY; ++ty) {
        for (int tx = startTileX; tx <= endTileX; ++tx) {
            if (level->GetTileAt(tx, ty)->IsSolid()) {
                // COLLISIONE RILEVATA -> RISOLUZIONE
                if (state.velocity.x > 0) { // Mi muovevo a destra
                    state.position.x = (float)(tx * TILE_SIZE) - PLAYER_WIDTH;
                    state.velocity.x = 0;
                }
                else if (state.velocity.x < 0) { // Mi muovevo a sinistra
                    state.position.x = (float)((tx + 1) * TILE_SIZE);
                    state.velocity.x = 0;
                }
                // Una volta risolto l'asse X per questa posizione, 
                // possiamo uscire dal loop X per evitare tremolii.
                return;
            }
        }
    }
}

void Player::ResolveCollisionsY()
{
    // Calcoliamo i limiti del player (AABB)
    float left = state.position.x;
    float right = state.position.x + PLAYER_WIDTH;
    float top = state.position.y;
    float bottom = state.position.y + PLAYER_HEIGHT;

    // Troviamo il range di tile da controllare (ottimizzazione)
    int startTileX = (int)(left / TILE_SIZE);
    int endTileX = (int)(right / TILE_SIZE);
    int startTileY = (int)(top / TILE_SIZE);
    int endTileY = (int)(bottom / TILE_SIZE);

    for (int ty = startTileY; ty <= endTileY; ++ty) {
        for (int tx = startTileX; tx <= endTileX; ++tx) {
            if (level->GetTileAt(tx, ty)->IsSolid()) {
                // COLLISIONE RILEVATA -> RISOLUZIONE
                if (state.velocity.y > 0) { // Mi muovevo verso il basso
                    state.position.y = (float)(ty * TILE_SIZE) - PLAYER_HEIGHT;
                    state.velocity.y = 0;
                }
                else if (state.velocity.y < 0) { // Mi muovevo verso l'alto
                    state.position.y = (float)((ty + 1) * TILE_SIZE);
                    state.velocity.y = 0;
                }
                // Una volta risolto l'asse Y per questa posizione, uscire dal loop
                return;
            }
        }
    }
}