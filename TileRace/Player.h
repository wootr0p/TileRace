#pragma once
#include <SDL3/SDL.h>

struct Vec2 {
    float x, y;

    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    Vec2 operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }
};

class InputManager;

class Player {
public:
    Player();
    ~Player();

    void Update(float dt, InputManager* input);
    void Render(SDL_Renderer* renderer);

    Vec2 GetPosition() const { return position; }
    Vec2 GetVelocity() const { return velocity; }

    bool IsOnGround() const { return is_on_ground; }
    bool CanDash() const { return can_dash; }
    bool IsDashing() const { return dash_timer > 0; }

    const char* GetDebugText() const { return debug_text; }

private:
    // Physics
    void UpdatePhysics(float dt, InputManager* input);
    void HandleCollisions();
    void UpdateDash(float dt, InputManager* input);
    void HandleJump(InputManager* input);
    void HandleWallJump(InputManager* input);

    Vec2 position;
    Vec2 velocity;
    Vec2 dash_direction;

    bool is_on_ground;
    bool old_is_on_ground;
    bool just_on_air;
    bool is_on_wall_left, is_on_wall_right;
    bool can_wall_jump_left, can_wall_jump_right;
    bool is_wall_jumping_left, is_wall_jumping_right;
    bool can_dash;
    bool jump_down;

    // Timers
    float jump_buffer_timer;
    float wall_jump_buffer_timer;
    float wall_jump_inhibition_timer;
    float dash_timer;
    float dash_cooldown_timer;

    char debug_text[255];

    // Physics parameters
    static const float GRAVITY;
    static const float JUMP_FORCE;
    static const float WALL_JUMP_FORCE;
    static const float WALL_JUMP_FORCE_X;
    static const float JUMP_RELEASE_FORCE;
    static const float JUMP_BUFFER_TIME;
    static const float WALL_JUMP_BUFFER_TIME;
    static const float WALL_JUMP_INHIBITION_TIME;
    static const float ACCEL_X;
    static const float FRICTION_X;
    static const float MAX_VEL_X;
    static const float MAX_FALL_SPEED;

    // Dash parameters
    static const float DASH_VELOCITY;
    static const float DASH_DURATION;
    static const float DASH_COOLDOWN;

    // Visual parameters
    static const int WIDTH;
    static const int HEIGHT;
    static const int WINDOW_WIDTH;
    static const int WINDOW_HEIGHT;
};
