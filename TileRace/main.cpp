#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>

using namespace std;

// --- CONFIGURAZIONE SISTEMA ---
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define PLAYER_WIDTH 32
#define PLAYER_HEIGHT 32
#define MAX_TRAIL_GHOSTS 30
#define TRAIL_EMIT_RATE 0.012f 
#define FPS_SAMPLES 60

// --- PARAMETRI FISICI ---
static const float GRAVITY = 2800.0f;
static const float JUMP_FORCE = -1100.0f;
static const float WALL_JUMP_FORCE = -1100.0f;
static const float WALL_JUMP_FORCE_X = -4000.0f;
static const float JUMP_RELEASE_FORCE = 3.8f;
static const float JUMP_BUFFER_TIME = 0.15f;
static const float WALL_JUMP_BUFFER_TIME = 0.15f;
static const float WALL_JUMP_INIBITION_TIME = 0.1f;
static const float ACCEL_X = 24000.0f;
static const float FRICTION_X = 5000.0f;
static const float MAX_VEL_X = 500.0f;
static const float MAX_FALL_SPEED = 1200.0f;


// --- PARAMETRI DASH ---
static const float DASH_VELOCITY = 1600.0f;
static const float DASH_DURATION = 0.14f;
static const float DASH_COOLDOWN = 0.15f;
static const float ANALOG_THRESHOLD = 0.25f;

typedef struct {
    float x, y;
    float alpha;
    bool active;
} TrailGhost;

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Gamepad* gamepad = NULL;
static uint64_t last_ticks = 0;

static float fps_buffer[FPS_SAMPLES];
static int fps_index = 0;
static char fps_text[32] = "fps: 0";
static char debug_text[255] = "";
static uint64_t fps_update_timer = 0;

static float player_x = 0.0f, player_y = 0.0f;
static float vel_x = 0.0f, vel_y = 0.0f;
static bool is_on_ground = true;
static bool old_is_on_ground = true;
static bool just_on_air = false;
static bool is_on_wall_left = false, is_on_wall_right = false;
static bool can_wall_jump_left = true, can_wall_jump_right = true;
static bool is_wall_jumping_left = false, is_wall_jumping_right = false;
static bool can_dash = true;
static bool jump_down = false;
static float jump_buffer_timer = 0.0f;
static float wall_jump_buffer_timer = 0.0f;
static float wall_jump_inibition_timer = 0.0f;


static float dash_timer = 0.0f;
static float dash_cooldown_timer = 0.0f;
static float dash_v_x = 0.0f, dash_v_y = 0.0f; // Direzione del dash corrente

static TrailGhost trail[MAX_TRAIL_GHOSTS];
static float trail_emit_timer = 0.0f;

void GetRawInput(float* out_x, float* out_y) {
    float rx = 0.0f, ry = 0.0f;
    const bool* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) rx -= 1.0f;
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) rx += 1.0f;
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) ry -= 1.0f;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) ry += 1.0f;

    if (gamepad) {
        float ax = (float)SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f;
        float ay = (float)SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f;
        if (SDL_sqrtf(ax * ax + ay * ay) > ANALOG_THRESHOLD) { rx = ax; ry = ay; }

        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT)) rx -= 1.0f;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) rx += 1.0f;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP)) ry -= 1.0f;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN)) ry += 1.0f;
    }

    float mag = SDL_sqrtf(rx * rx + ry * ry);
    if (mag > 0.1f) { *out_x = rx / mag; *out_y = ry / mag; }
    else { *out_x = 0.0f; *out_y = 0.0f; }
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
    SDL_CreateWindowAndRenderer("Demo PS", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    player_x = (WINDOW_WIDTH - PLAYER_WIDTH) / 2.0f;
    player_y = WINDOW_HEIGHT - PLAYER_HEIGHT;
    last_ticks = SDL_GetTicksNS();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;
    if (event->type == SDL_EVENT_GAMEPAD_ADDED) gamepad = SDL_OpenGamepad(event->gdevice.which);
    if (event->type == SDL_EVENT_GAMEPAD_REMOVED) gamepad = NULL;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    uint64_t current_ticks = SDL_GetTicksNS();
    float dt = SDL_min((float)(current_ticks - last_ticks) / 1e9f, 0.05f);
    last_ticks = current_ticks;
    old_is_on_ground = is_on_ground;

    // FPS Telemetry
    fps_buffer[fps_index] = dt;
    fps_index = (fps_index + 1) % FPS_SAMPLES;
    if (current_ticks > fps_update_timer) {
        float avg_dt = 0;
        for (int i = 0; i < FPS_SAMPLES; i++) avg_dt += fps_buffer[i];
        SDL_snprintf(fps_text, sizeof(fps_text), "fps: %.0f", (float)FPS_SAMPLES / avg_dt);
        fps_update_timer = current_ticks + 500000000;
    }

    if (dash_timer > 0) dash_timer -= dt;
    if (dash_cooldown_timer > 0) dash_cooldown_timer -= dt;
    if (jump_buffer_timer > 0) jump_buffer_timer -= dt;
    if (wall_jump_inibition_timer > 0) wall_jump_inibition_timer -= dt;
    if (wall_jump_buffer_timer > 0) wall_jump_buffer_timer -= dt;

    float in_x, in_y;
    GetRawInput(&in_x, &in_y);

    const bool* keys = SDL_GetKeyboardState(NULL);
    bool new_jump_down = keys[SDL_SCANCODE_SPACE] || (gamepad && SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH));
    bool jump_just_pressed = (jump_down == false && new_jump_down == true);
    jump_down = new_jump_down;

    bool dash_down = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT] || (gamepad && SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST));

    if (jump_just_pressed) {
        jump_buffer_timer = JUMP_BUFFER_TIME; // Jump Buffer
        wall_jump_buffer_timer = WALL_JUMP_BUFFER_TIME; // Wall Jump Buffer
    }
    if (just_on_air) {
        wall_jump_buffer_timer = 0;
    }

    // START DASH
    if (dash_down && can_dash && dash_timer <= 0 && dash_cooldown_timer <= 0) {
        // Direzione di default del dash
        if (in_x == 0.0f && in_y == 0.0f) { dash_v_x = 0.0f; dash_v_y = -1.0f; }
        else { dash_v_x = in_x; dash_v_y = in_y; }

        dash_timer = DASH_DURATION;
        dash_cooldown_timer = DASH_DURATION + DASH_COOLDOWN;
        can_dash = false;
        is_on_ground = false;
    }

    // PHYSICS
    if (dash_timer > 0) {
        // ACTIVE STEERING: Cambia direzione mentre dash-i!
        if (SDL_fabsf(in_x) > 0.1f || SDL_fabsf(in_y) > 0.1f) {
            dash_v_x = in_x;
            dash_v_y = in_y;
        }
        vel_x = dash_v_x * DASH_VELOCITY;
        vel_y = dash_v_y * DASH_VELOCITY;
    }
    else {

        float move_dir = 0;

        // Se non siamo inibiti da un wall jump recente, permettiamo input orizzontale normale, altrimenti accelero verso la direzione del wall jump.
        if (wall_jump_inibition_timer <= 0) {
            is_wall_jumping_left = false;
            is_wall_jumping_right = false;

            move_dir = (in_x > 0.1f) ? 1.0f : (in_x < -0.1f ? -1.0f : 0.0f);
            if (move_dir != 0) vel_x += move_dir * ACCEL_X * dt;
            else {
                if (vel_x > 0) vel_x = SDL_max(0, vel_x - FRICTION_X * dt);
                else if (vel_x < 0) vel_x = SDL_min(0, vel_x + FRICTION_X * dt);
            }
        }
        else {
            if (is_wall_jumping_left) {
                vel_x += ACCEL_X * dt;
            }
            else if (is_wall_jumping_right) {
                vel_x += -ACCEL_X * dt;
            }
        }


        vel_x = SDL_clamp(vel_x, -MAX_VEL_X, MAX_VEL_X);

        // SALTO A MURO
        if (!is_on_ground && wall_jump_buffer_timer > 0) {
            if (can_wall_jump_left && is_on_wall_left) {
                vel_y = WALL_JUMP_FORCE;
                vel_x += -WALL_JUMP_FORCE_X;
                can_wall_jump_left = false;
                wall_jump_inibition_timer = WALL_JUMP_INIBITION_TIME;
                is_wall_jumping_left = true;
                can_wall_jump_right = true;
            }
            else if (can_wall_jump_right && is_on_wall_right) {
                vel_y = WALL_JUMP_FORCE;
                vel_x += WALL_JUMP_FORCE_X;
                can_wall_jump_right = false;
                wall_jump_inibition_timer = WALL_JUMP_INIBITION_TIME;
                is_wall_jumping_right = true;
                can_wall_jump_left = true;
            }
        }

        // SALTO NORMALE
        if (jump_buffer_timer > 0 && is_on_ground) { vel_y = JUMP_FORCE; is_on_ground = false; }


#ifdef _DEBUG
        SDL_snprintf(debug_text, sizeof(debug_text), "can_wj_left: %d | can_wj_right: %d | jump_buffer_timer: %f | wj_inibition_timer: %f", can_wall_jump_left, can_wall_jump_right, jump_buffer_timer, wall_jump_inibition_timer);
#endif


        float grav_mult = (vel_y < 0 && !jump_down) ? JUMP_RELEASE_FORCE : 1.0f;
        vel_y += (GRAVITY * grav_mult) * dt;

        if (vel_y > MAX_FALL_SPEED) vel_y = MAX_FALL_SPEED;
    }

    player_x += vel_x * dt;
    player_y += vel_y * dt;

    // COLLISIONS
    if (player_y >= WINDOW_HEIGHT - PLAYER_HEIGHT) {
        player_y = WINDOW_HEIGHT - PLAYER_HEIGHT;
        vel_y = 0;
        is_on_ground = true;
        can_dash = true;
        can_wall_jump_left = true;
        can_wall_jump_right = true;
    }
    else if (player_y <= 0) {
        player_y = 0;
        vel_y = 0;
    }

    just_on_air = (old_is_on_ground && !is_on_ground);


    if (player_x <= 0) { player_x = 0; vel_x = 0; is_on_wall_left = true; }
    else {
        is_on_wall_left = false;
    }
    if (player_x >= WINDOW_WIDTH - PLAYER_WIDTH) { player_x = WINDOW_WIDTH - PLAYER_WIDTH; vel_x = 0; is_on_wall_right = true; }
    else {
        is_on_wall_right = false;
    }

    // RENDERING
    SDL_SetRenderDrawColor(renderer, 10, 10, 15, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < MAX_TRAIL_GHOSTS; i++) {
        if (trail[i].active) {
            trail[i].alpha -= 4.2f * dt;
            if (trail[i].alpha <= 0) trail[i].active = false;
            else {
                SDL_FRect r = { trail[i].x, trail[i].y, (float)PLAYER_WIDTH, (float)PLAYER_HEIGHT };
                SDL_SetRenderDrawColor(renderer, 255, 120, 0, (Uint8)(trail[i].alpha * 180));
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }

    trail_emit_timer -= dt;
    if (dash_timer > 0 && trail_emit_timer <= 0) {
        for (int i = 0; i < MAX_TRAIL_GHOSTS; i++) {
            if (!trail[i].active) {
                trail[i].x = player_x; trail[i].y = player_y;
                trail[i].alpha = 0.6f; trail[i].active = true;
                trail_emit_timer = TRAIL_EMIT_RATE; break;
            }
        }
    }

    SDL_FRect p_rect = { player_x, player_y, (float)PLAYER_WIDTH, (float)PLAYER_HEIGHT };
    if (can_dash) SDL_SetRenderDrawColor(renderer, 0, 255, 180, 255);
    else SDL_SetRenderDrawColor(renderer, 255, 100, 0, 255);
    SDL_RenderFillRect(renderer, &p_rect);

    SDL_SetRenderScale(renderer, 2.0f, 2.0f);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, 5, 5, fps_text);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);
    SDL_RenderDebugText(renderer, 5, WINDOW_HEIGHT - 15, debug_text);

    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) { SDL_Quit(); }