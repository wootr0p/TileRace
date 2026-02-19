#include "Player.h"
#include "../engine/InputManager.h"

// Physics parameters
const float Player::GRAVITY = 2800.0f;
const float Player::JUMP_FORCE = -1100.0f;
const float Player::WALL_JUMP_FORCE = -1100.0f;
const float Player::WALL_JUMP_FORCE_X = -4000.0f;
const float Player::JUMP_RELEASE_FORCE = 3.8f;
const float Player::JUMP_BUFFER_TIME = 0.15f;
const float Player::WALL_JUMP_BUFFER_TIME = 0.15f;
const float Player::WALL_JUMP_INHIBITION_TIME = 0.1f;
const float Player::ACCEL_X = 24000.0f;
const float Player::FRICTION_X = 5000.0f;
const float Player::MAX_VEL_X = 500.0f;
const float Player::MAX_FALL_SPEED = 1200.0f;

// Dash parameters
const float Player::DASH_VELOCITY = 1600.0f;
const float Player::DASH_DURATION = 0.14f;
const float Player::DASH_COOLDOWN = 0.15f;

// Visual parameters
const int Player::WIDTH = 32;
const int Player::HEIGHT = 32;
const int Player::WINDOW_WIDTH = 1280;
const int Player::WINDOW_HEIGHT = 720;

Player::Player()
    : position((WINDOW_WIDTH - WIDTH) / 2.0f, WINDOW_HEIGHT - HEIGHT)
    , velocity(0, 0)
    , dash_direction(0, -1)
    , is_on_ground(true)
    , old_is_on_ground(true)
    , just_on_air(false)
    , is_on_wall_left(false)
    , is_on_wall_right(false)
    , can_wall_jump_left(true)
    , can_wall_jump_right(true)
    , is_wall_jumping_left(false)
    , is_wall_jumping_right(false)
    , can_dash(true)
    , jump_down(false)
    , jump_buffer_timer(0.0f)
    , wall_jump_buffer_timer(0.0f)
    , wall_jump_inhibition_timer(0.0f)
    , dash_timer(0.0f)
    , dash_cooldown_timer(0.0f)
{
    SDL_snprintf(debug_text, sizeof(debug_text), "");
}

Player::~Player() {
}

void Player::Update(float dt, InputManager* input) {
    old_is_on_ground = is_on_ground;

    // Update timers
    if (dash_timer > 0) dash_timer -= dt;
    if (dash_cooldown_timer > 0) dash_cooldown_timer -= dt;
    if (jump_buffer_timer > 0) jump_buffer_timer -= dt;
    if (wall_jump_inhibition_timer > 0) wall_jump_inhibition_timer -= dt;
    if (wall_jump_buffer_timer > 0) wall_jump_buffer_timer -= dt;

    // Update input state
    input->Update();

    // Handle jump buffer and wall jump buffer
    if (input->IsJumpJustPressed()) {
        jump_buffer_timer = JUMP_BUFFER_TIME;
        wall_jump_buffer_timer = WALL_JUMP_BUFFER_TIME;
    }
    if (just_on_air) {
        wall_jump_buffer_timer = 0;
    }

    // Update dash
    UpdateDash(dt, input);

    // Update physics
    UpdatePhysics(dt, input);

    // Update position
    position = position + velocity * dt;

    // Handle collisions
    HandleCollisions();

    just_on_air = (old_is_on_ground && !is_on_ground);

#ifdef _DEBUG
    SDL_snprintf(debug_text, sizeof(debug_text),
        "can_wj_left: %d | can_wj_right: %d | jump_buffer: %.2f | wj_inhibition: %.2f",
        can_wall_jump_left, can_wall_jump_right, jump_buffer_timer, wall_jump_inhibition_timer);
#endif
}

void Player::UpdateDash(float dt, InputManager* input) {
    bool dash_down = input->IsDashDown();

    // Start dash
    if (dash_down && can_dash && dash_timer <= 0 && dash_cooldown_timer <= 0) {
        float in_x, in_y;
        input->GetRawInput(&in_x, &in_y);

        // Default dash direction is upward
        if (in_x == 0.0f && in_y == 0.0f) {
            dash_direction = Vec2(0.0f, -1.0f);
        } else {
            dash_direction = Vec2(in_x, in_y);
        }

        dash_timer = DASH_DURATION;
        dash_cooldown_timer = DASH_DURATION + DASH_COOLDOWN;
        can_dash = false;
        is_on_ground = false;
    }
}

void Player::UpdatePhysics(float dt, InputManager* input) {
    float in_x, in_y;
    input->GetRawInput(&in_x, &in_y);

    if (dash_timer > 0) {
        // Active steering: change direction during dash
        if (SDL_fabsf(in_x) > 0.1f || SDL_fabsf(in_y) > 0.1f) {
            dash_direction = Vec2(in_x, in_y);
        }
        velocity.x = dash_direction.x * DASH_VELOCITY;
        velocity.y = dash_direction.y * DASH_VELOCITY;
    } else {
        float move_dir = 0;

        // Wall jump inhibition handling
        if (wall_jump_inhibition_timer <= 0) {
            is_wall_jumping_left = false;
            is_wall_jumping_right = false;

            move_dir = (in_x > 0.1f) ? 1.0f : (in_x < -0.1f ? -1.0f : 0.0f);
            if (move_dir != 0) {
                velocity.x += move_dir * ACCEL_X * dt;
            } else {
                if (velocity.x > 0) velocity.x = SDL_max(0.0f, velocity.x - FRICTION_X * dt);
                else if (velocity.x < 0) velocity.x = SDL_min(0.0f, velocity.x + FRICTION_X * dt);
            }
        } else {
            if (is_wall_jumping_left) {
                velocity.x += ACCEL_X * dt;
            } else if (is_wall_jumping_right) {
                velocity.x += -ACCEL_X * dt;
            }
        }

        velocity.x = SDL_clamp(velocity.x, -MAX_VEL_X, MAX_VEL_X);

        // Handle wall jump
        HandleWallJump(input);

        // Handle normal jump
        HandleJump(input);

        // Apply gravity
        float grav_mult = (velocity.y < 0 && !input->IsJumpDown()) ? JUMP_RELEASE_FORCE : 1.0f;
        velocity.y += (GRAVITY * grav_mult) * dt;

        if (velocity.y > MAX_FALL_SPEED) velocity.y = MAX_FALL_SPEED;
    }
}

void Player::HandleWallJump(InputManager* input) {
    if (!is_on_ground && wall_jump_buffer_timer > 0) {
        if (can_wall_jump_left && is_on_wall_left) {
            velocity.y = WALL_JUMP_FORCE;
            velocity.x += -WALL_JUMP_FORCE_X;
            can_wall_jump_left = false;
            wall_jump_inhibition_timer = WALL_JUMP_INHIBITION_TIME;
            is_wall_jumping_left = true;
            can_wall_jump_right = true;
        } else if (can_wall_jump_right && is_on_wall_right) {
            velocity.y = WALL_JUMP_FORCE;
            velocity.x += WALL_JUMP_FORCE_X;
            can_wall_jump_right = false;
            wall_jump_inhibition_timer = WALL_JUMP_INHIBITION_TIME;
            is_wall_jumping_right = true;
            can_wall_jump_left = true;
        }
    }
}

void Player::HandleJump(InputManager* input) {
    if (jump_buffer_timer > 0 && is_on_ground) {
        velocity.y = JUMP_FORCE;
        is_on_ground = false;
    }
}

void Player::HandleCollisions() {
    // Ground collision
    if (position.y >= WINDOW_HEIGHT - HEIGHT) {
        position.y = WINDOW_HEIGHT - HEIGHT;
        velocity.y = 0;
        is_on_ground = true;
        can_dash = true;
        can_wall_jump_left = true;
        can_wall_jump_right = true;
    } else if (position.y <= 0) {
        position.y = 0;
        velocity.y = 0;
    }

    // Wall collision
    if (position.x <= 0) {
        position.x = 0;
        velocity.x = 0;
        is_on_wall_left = true;
    } else {
        is_on_wall_left = false;
    }

    if (position.x >= WINDOW_WIDTH - WIDTH) {
        position.x = WINDOW_WIDTH - WIDTH;
        velocity.x = 0;
        is_on_wall_right = true;
    } else {
        is_on_wall_right = false;
    }
}

void Player::Render(SDL_Renderer* renderer) {
    SDL_FRect p_rect = { position.x, position.y, (float)WIDTH, (float)HEIGHT };

    if (can_dash) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 180, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 100, 0, 255);
    }

    SDL_RenderFillRect(renderer, &p_rect);
}
