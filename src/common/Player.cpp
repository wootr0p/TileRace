#include "Player.h"
#include "World.h"
#include "Physics.h"
#include <cmath>

Player::Player(uint32_t id) {
    state_.player_id = id;
}

void Player::SetState(const PlayerState& state) {
    state_ = state;
}

void Player::Simulate(const InputFrame& input, const World& world) {
    // ==========================================================================
    // ORDINE CANONICO:
    //   1. Cooldown tick
    //   2. Input orizzontale + last_dir
    //   3. Dash
    //   4. Salto (ground) / Wall-jump
    //   5. Gravità (ridotta durante dash)
    //   6. Muovi X → ResolveX (con wall detection)
    //   7. Muovi Y → ResolveY
    // ==========================================================================

    // 1. Tick cooldown/durata
    if (state_.dash_cooldown_ticks > 0)  --state_.dash_cooldown_ticks;
    if (state_.dash_active_ticks   > 0)  --state_.dash_active_ticks;

    // Salva i flag muro del tick PRECEDENTE prima di resettarli.
    // Il reset avviene qui ma il check salto deve usare i contatti già noti,
    // perché ResolveCollisionsX (che li ri-rileva) viene DOPO il jump check.
    const bool was_on_wall_left  = state_.on_wall_left;
    const bool was_on_wall_right = state_.on_wall_right;

    // Reset flag muro (verranno rilevati in ResolveCollisionsX)
    state_.on_wall_left  = false;
    state_.on_wall_right = false;

    // 2. Input orizzontale
    if (!state_.dash_active_ticks) {
        // Normale: vx da input
        if      (input.IsLeft())  { state_.vel_x = -Physics::MOVE_SPEED; state_.last_dir = -1; }
        else if (input.IsRight()) { state_.vel_x =  Physics::MOVE_SPEED; state_.last_dir =  1; }
        else                      { state_.vel_x = 0.0f; }
    }
    // Durante dash: vel_x mantenuta dal launch (non sovrascrivere)

    // 3. Dash — consuma BTN_DASH solo se cooldown esaurito e non già in dash
    if (input.IsDash() && state_.dash_cooldown_ticks == 0 && state_.dash_active_ticks == 0) {
        // Direzione: last_dir, o input corrente, o 1 se fermo
        int8_t dir = state_.last_dir;
        if (input.IsLeft())       dir = -1;
        else if (input.IsRight()) dir =  1;
        if (dir == 0)             dir =  1;

        state_.vel_x              = Physics::DASH_SPEED * static_cast<float>(dir);
        state_.vel_y              = 0.0f;   // annulla caduta all'inizio del dash
        state_.dash_active_ticks  = Physics::DASH_DURATION_TICKS;
        state_.dash_cooldown_ticks = Physics::DASH_COOLDOWN_TICKS;
        state_.last_dir           = dir;
    }

    // 4. Salto
    if (input.IsJump()) {
        if (state_.on_ground) {
            // Salto normale
            state_.vel_y     = Physics::JUMP_FORCE;
            state_.on_ground = false;
        } else if (was_on_wall_left || was_on_wall_right) {
            // Wall-jump: velocità verticale + orizzontale via dal muro
            state_.vel_y = Physics::WALL_JUMP_VEL_Y;
            if (was_on_wall_left) {
                state_.vel_x    = Physics::WALL_JUMP_VEL_X;  // salta a destra
                state_.last_dir =  1;
            } else {
                state_.vel_x    = -Physics::WALL_JUMP_VEL_X; // salta a sinistra
                state_.last_dir = -1;
            }
        }
    }

    // 5. Gravità (azzerata durante dash attivo)
    if (!state_.dash_active_ticks) {
        state_.vel_y += Physics::GRAVITY * Physics::FIXED_DT;
        if (state_.vel_y > Physics::MAX_FALL_SPEED)
            state_.vel_y = Physics::MAX_FALL_SPEED;
    }

    // 6. Muovi X → ResolveX (setta on_wall_left / on_wall_right)
    state_.x += state_.vel_x * Physics::FIXED_DT;
    ResolveCollisionsX(world);

    // 7. Muovi Y → ResolveY
    state_.on_ground = false;
    state_.y += state_.vel_y * Physics::FIXED_DT;
    ResolveCollisionsY(world);

    // Se durante il dash tocchiamo un muro, interrompiamo il dash
    if (state_.dash_active_ticks > 0 && (state_.on_wall_left || state_.on_wall_right)) {
        state_.dash_active_ticks = 0;
        state_.vel_x = 0.0f;
    }

    state_.last_processed_tick = input.tick;
}

void Player::ResolveCollisionsX(const World& world) {
    const float ts  = Physics::TILE_SIZE;

    // INSET verticale di X_INSET pixel: evita falsi positivi con il tile del pavimento.
    // Senza inset, se state_.y = 64.001 (after gravity dip), ty1 = floor(96.001/32) = 3
    // che include la riga del pavimento → rilevata come muro → vel_x=0 (jitter).
    int ty0 = static_cast<int>(std::floor((state_.y + X_INSET) / ts));
    int ty1 = static_cast<int>(std::floor((state_.y + PLAYER_H - X_INSET) / ts));

    if (state_.vel_x < 0.0f) {
        int tx = static_cast<int>(std::floor(state_.x / ts));
        bool hit = false;
        for (int ty = ty0; ty <= ty1; ++ty) {
            if (world.IsSolid(tx, ty)) { hit = true; break; }
        }
        if (hit) {
            state_.x            = static_cast<float>(tx + 1) * ts;
            state_.vel_x        = 0.0f;
            state_.on_wall_left = true;
        }
    } else if (state_.vel_x > 0.0f) {
        int tx = static_cast<int>(std::floor((state_.x + PLAYER_W - 0.01f) / ts));
        bool hit = false;
        for (int ty = ty0; ty <= ty1; ++ty) {
            if (world.IsSolid(tx, ty)) { hit = true; break; }
        }
        if (hit) {
            state_.x             = static_cast<float>(tx) * ts - PLAYER_W;
            state_.vel_x         = 0.0f;
            state_.on_wall_right = true;
        }
    }

    // Wall-detection anche quando vel_x == 0 (player fermo appoggiato al muro):
    // controlla un tile a sinistra e a destra senza spostare il player.
    if (!state_.on_wall_left) {
        int tx = static_cast<int>(std::floor((state_.x - 1.0f) / ts));
        for (int ty = ty0; ty <= ty1; ++ty) {
            if (world.IsSolid(tx, ty)) { state_.on_wall_left = true; break; }
        }
    }
    if (!state_.on_wall_right) {
        int tx = static_cast<int>(std::floor((state_.x + PLAYER_W) / ts));
        for (int ty = ty0; ty <= ty1; ++ty) {
            if (world.IsSolid(tx, ty)) { state_.on_wall_right = true; break; }
        }
    }
}

void Player::ResolveCollisionsY(const World& world) {
    const float ts  = Physics::TILE_SIZE;
    int tx0 = static_cast<int>(std::floor(state_.x / ts));
    int tx1 = static_cast<int>(std::floor((state_.x + PLAYER_W - 0.01f) / ts));

    if (state_.vel_y > 0.0f) {
        int ty = static_cast<int>(std::floor((state_.y + PLAYER_H - 0.01f) / ts));
        bool hit = false;
        for (int tx = tx0; tx <= tx1; ++tx) {
            if (world.IsSolid(tx, ty)) { hit = true; break; }
        }
        if (hit) {
            state_.y         = static_cast<float>(ty) * ts - PLAYER_H;
            state_.vel_y     = 0.0f;
            state_.on_ground = true;
        }
    } else if (state_.vel_y < 0.0f) {
        int ty = static_cast<int>(std::floor(state_.y / ts));
        bool hit = false;
        for (int tx = tx0; tx <= tx1; ++tx) {
            if (world.IsSolid(tx, ty)) { hit = true; break; }
        }
        if (hit) {
            state_.y     = static_cast<float>(ty + 1) * ts;
            state_.vel_y = 0.0f;
        }
    }
}
