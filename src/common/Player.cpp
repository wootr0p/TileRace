#include "Player.h"
#include "InputFrame.h"
#include "World.h"
#include "Physics.h"
#include <cmath>    // fabsf, sqrtf

// ---------------------------------------------------------------------------
// Simulate — punto unico di aggiornamento (passo 9)
// ---------------------------------------------------------------------------
void Player::Simulate(const InputFrame& frame, const World& world) {
    // Registra il tick processato per permettere la reconciliation lato client.
    state_.last_processed_tick = frame.tick;

    // 1. Jump buffer (edge rising)
    if (frame.Has(BTN_JUMP_PRESS))
        RequestJump();

    // 2. Variable jump cut (edge falling: tasto rilasciato mentre si sale)
    const bool jump_held = frame.Has(BTN_JUMP);
    if (prev_jump_held_ && !jump_held)
        CutJump();
    prev_jump_held_ = jump_held;

    // 3. Avvio dash (edge rising)
    if (frame.Has(BTN_DASH))
        RequestDash(frame.dash_dx, frame.dash_dy);

    // 4. Sterzata dash in corso
    if (state_.dash_active_ticks > 0)
        SteerDash(frame.dash_dx, frame.dash_dy);

    // 5. Movimento orizzontale
    float dx = 0.f;
    if (frame.Has(BTN_LEFT))  dx -= MOVE_SPEED * FIXED_DT;
    if (frame.Has(BTN_RIGHT)) dx += MOVE_SPEED * FIXED_DT;
    MoveX(dx, world);

    // 6. Gravità, salto, collisioni Y
    MoveY(FIXED_DT, world);
}

// ---------------------------------------------------------------------------
// RequestJump / CutJump
// ---------------------------------------------------------------------------
void Player::RequestJump() {
    state_.jump_buffer_ticks = JUMP_BUFFER_TICKS;
}

void Player::CutJump() {
    if (state_.vel_y < 0.f)
        state_.vel_y *= JUMP_CUT_MULTIPLIER;
}

// ---------------------------------------------------------------------------
// RequestDash
// ---------------------------------------------------------------------------
void Player::RequestDash(float dx, float dy) {
    if (state_.dash_ready &&
        state_.dash_cooldown_ticks == 0 && state_.dash_active_ticks == 0) {
        // Normalizza il vettore direzione a lunghezza 1.
        // Se (0,0) usa il fallback orizzontale last_dir.
        const float len = sqrtf(dx * dx + dy * dy);
        if (len > 0.001f) {
            state_.dash_dir_x = dx / len;
            state_.dash_dir_y = dy / len;
        } else {
            // Nessuna direzione specificata: dash verso l'alto di default.
            state_.dash_dir_x = 0.f;
            state_.dash_dir_y = -1.f;
        }
        state_.dash_active_ticks = static_cast<uint8_t>(DASH_ACTIVE_TICKS);
        state_.dash_ready         = false;  // carica consumata finché non si tocca terra
        state_.vel_x              = 0.f;
        state_.vel_y             = 0.f;
    }
}

// Aggiorna direzione del dash in corso (sterzata)
void Player::SteerDash(float dx, float dy) {
    if (state_.dash_active_ticks == 0) return;
    const float len = sqrtf(dx * dx + dy * dy);
    if (len > 0.001f) {
        state_.dash_dir_x = dx / len;
        state_.dash_dir_y = dy / len;
    }
    // Se input è (0,0) mantiene la direzione corrente
}

// ---------------------------------------------------------------------------
// MoveX: input + vel_x (wall jump impulse), dash, collisioni, wall probe, wall jump
// ---------------------------------------------------------------------------
void Player::MoveX(float input_dx, const World& world) {
    state_.on_wall_left  = false;
    state_.on_wall_right = false;

    // Aggiorna last_dir dalla direzione di input (usato dal dash)
    if      (input_dx > 0.f) state_.last_dir =  1;
    else if (input_dx < 0.f) state_.last_dir = -1;

    // Decadimento cooldown dash (una volta per tick)
    if (state_.dash_cooldown_ticks > 0)
        state_.dash_cooldown_ticks--;

    float total_dx;
    if (state_.dash_active_ticks > 0) {
        // --- Dash attivo: usa il vettore normalizzato ---
        total_dx = state_.dash_dir_x * DASH_SPEED * FIXED_DT;
        // Resetta l'inerzia alla velocità orizzontale corrente così non c'è scatto al termine
        state_.move_vel_x = state_.dash_dir_x * DASH_SPEED;
        // Il decremento di dash_active_ticks è gestito in MoveY (dopo la componente Y)
    } else {
        // --- Movimento con inerzia ---
        // Decadimento esponenziale dell'impulso orizzontale (wall jump kick)
        state_.vel_x -= state_.vel_x * WALL_JUMP_FRICTION * FIXED_DT;
        if (fabsf(state_.vel_x) < 1.f) state_.vel_x = 0.f;

        // Velocità target dall'input (px/s)
        const float target = input_dx / FIXED_DT;   // ±MOVE_SPEED o 0
        // Usa decelerazione maggiore se si inverte direzione o si ferma
        const float accel = (target == 0.f || (target * state_.move_vel_x < 0.f))
                            ? MOVE_DECEL : MOVE_ACCEL;
        const float delta      = target - state_.move_vel_x;
        const float max_change = accel * FIXED_DT;
        state_.move_vel_x += (delta > max_change) ? max_change
                           : (delta < -max_change) ? -max_change
                           : delta;

        total_dx = (state_.move_vel_x + state_.vel_x) * FIXED_DT;
    }

    state_.x += total_dx;
    ResolveCollisionsX(world, total_dx);   // snap + rileva on_wall_left/right

    // Wall jump: scatta se il buffer è attivo, il player è in aria appoggiato a un muro,
    // non ha già saltato da quel lato e non sta dashando.
    if (state_.dash_active_ticks == 0 &&
        state_.jump_buffer_ticks > 0 && !state_.on_ground) {
        if (state_.on_wall_right && state_.last_wall_jump_dir != 1) {
            state_.vel_y              = -JUMP_FORCE;
            state_.vel_x              = -WALL_JUMP_FORCE_X;   // spinge a sinistra
            state_.move_vel_x         = 0.f;  // azzera inerzia per dare piena forza al kick
            state_.jump_buffer_ticks  = 0;
            state_.last_wall_jump_dir = 1;
        } else if (state_.on_wall_left && state_.last_wall_jump_dir != -1) {
            state_.vel_y              = -JUMP_FORCE;
            state_.vel_x              =  WALL_JUMP_FORCE_X;   // spinge a destra
            state_.move_vel_x         = 0.f;  // azzera inerzia per dare piena forza al kick
            state_.jump_buffer_ticks  = 0;
            state_.last_wall_jump_dir = -1;
        }
    }
}

void Player::ResolveCollisionsX(const World& world, float dx) {
    const int inset = 2;   // px ignorati in alto e in basso (evita falso-muro sul pavimento)
    int ty_top = static_cast<int>(state_.y + inset)                 / TILE_SIZE;
    int ty_bot = static_cast<int>(state_.y + TILE_SIZE - 1 - inset) / TILE_SIZE;

    // --- Snap (solo se ci si sta muovendo verso quel lato) ---
    if (dx > 0.f) {
        int tx = static_cast<int>(state_.x + TILE_SIZE - 1) / TILE_SIZE;
        for (int ty = ty_top; ty <= ty_bot; ty++) {
            if (world.IsSolid(tx, ty)) {
                state_.x = static_cast<float>(tx * TILE_SIZE - TILE_SIZE);
                break;
            }
        }
    } else if (dx < 0.f) {
        int tx = static_cast<int>(state_.x) / TILE_SIZE;
        for (int ty = ty_top; ty <= ty_bot; ty++) {
            if (world.IsSolid(tx, ty)) {
                state_.x = static_cast<float>((tx + 1) * TILE_SIZE);
                break;
            }
        }
    }

    // --- Wall probe: rileva muri entro WALL_PROBE_REACH px dal bordo del player ---
    // Il probe si estende WALL_PROBE_REACH px oltre il bordo fisico così on_wall_left/right
    // scatta già quando il player è a 1/4 di tile dal muro, rendendo il wall jump
    // più facile da eseguire senza dover toccare perfettamente la parete.
    {
        // Tile a destra: primo pixel =  bordo destro + WALL_PROBE_REACH
        int tx_r = static_cast<int>(state_.x + TILE_SIZE + WALL_PROBE_REACH) / TILE_SIZE;
        for (int ty = ty_top; ty <= ty_bot; ty++)
            if (world.IsSolid(tx_r, ty)) { state_.on_wall_right = true; break; }
    }
    {
        // Tile a sinistra: primo pixel = bordo sinistro - 1 - WALL_PROBE_REACH
        int tx_l = (static_cast<int>(state_.x) - 1 - WALL_PROBE_REACH) / TILE_SIZE;
        for (int ty = ty_top; ty <= ty_bot; ty++)
            if (world.IsSolid(tx_l, ty)) { state_.on_wall_left = true; break; }
    }
}

// ---------------------------------------------------------------------------
// MoveY: coyote time, salto bufferizzato, gravità, collisioni Y
// ---------------------------------------------------------------------------
void Player::MoveY(float dt, const World& world) {
    // --- Durante il dash: applica componente Y, poi sospendi gravità e logica di salto ---
    if (state_.dash_active_ticks > 0) {
        if (state_.dash_dir_y != 0.f) {
            const float dy = state_.dash_dir_y * DASH_SPEED * FIXED_DT;
            state_.y    += dy;
            // IMPORTANTE: ResolveCollisionsY si basa sul segno di vel_y per sapere
            // quale bordo controllare. Durante il dash vel_y è 0, quindi va impostato
            // temporaneamente al segno del movimento altrimenti la collisione viene ignorata.
            state_.vel_y = dy;
            ResolveCollisionsY(world);
        }
        state_.vel_y = 0.f;  // resetta durante il dash
        // Decrementa il contatore e avvia il cooldown a fine dash
        state_.dash_active_ticks--;
        if (state_.dash_active_ticks == 0) {
            state_.dash_cooldown_ticks = static_cast<uint8_t>(DASH_COOLDOWN_TICKS);
            // Se il dash era verso il basso, trasferisci la velocità verticale anziché
            // resettarla a 0: evita la pausa "galleggiante" post-dash in caduta.
            if (state_.dash_dir_y > 0.f)
                state_.vel_y = state_.dash_dir_y * DASH_SPEED;
        }
        return;
    }

    const bool was_on_ground = state_.on_ground;
    state_.on_ground = false;

    // --- Coyote time ---
    // Refresh mentre a terra; decrementa quando in aria
    if (was_on_ground) {
        state_.coyote_ticks = static_cast<uint8_t>(COYOTE_TICKS);
    } else if (state_.coyote_ticks > 0) {
        state_.coyote_ticks--;
    }

    // --- Salto normale (con buffer + coyote) ---
    // Eseguito PRIMA della gravità per risposta immediata
    if (state_.jump_buffer_ticks > 0 && (was_on_ground || state_.coyote_ticks > 0)) {
        state_.vel_y              = -JUMP_FORCE;
        state_.coyote_ticks       = 0;
        state_.jump_buffer_ticks  = 0;
        state_.last_wall_jump_dir = 0;  // a terra: reset, può saltare su qualsiasi muro
    }

    // --- Gravità e integrazione ---
    if (was_on_ground && state_.vel_y >= 0.f) {
        // Già a terra e nessun salto attivo: usa un probe di 1 px verso il basso invece
        // di accumulare gravità ogni tick. Elimina il jitter vel_y~0→23→0 e on_ground
        // che oscillava tra true/false. Se il tile sotto sparisce (bordo), on_ground rimane
        // false e la caduta libera inizia normalmente dal tick successivo.
        state_.vel_y  = 1.f;  // valore minimo positivo perché ResolveCollisionsY usi il ramo "caduta"
        state_.y     += 1.f;
        ResolveCollisionsY(world);
        if (!state_.on_ground)
            state_.vel_y = 0.f;  // camminato fuori dal bordo: caduta libera da vel_y=0
    } else {
        state_.vel_y += GRAVITY * dt;
        if (state_.vel_y > MAX_FALL_SPEED)
            state_.vel_y = MAX_FALL_SPEED;
        state_.y += state_.vel_y * dt;
        ResolveCollisionsY(world);
    }

    // --- Decrementa jump buffer (se non è stato consumato sopra) ---
    if (state_.jump_buffer_ticks > 0)
        state_.jump_buffer_ticks--;
}

void Player::ResolveCollisionsY(const World& world) {
    const int inset = 1;   // px ignorati a sinistra e destra (evita incastro negli angoli)
    int tx_left  = static_cast<int>(state_.x + inset)                 / TILE_SIZE;
    int tx_right = static_cast<int>(state_.x + TILE_SIZE - 1 - inset) / TILE_SIZE;

    if (state_.vel_y > 0.f) {
        // Caduta: controlla bordo inferiore
        int ty = static_cast<int>(state_.y + TILE_SIZE - 1) / TILE_SIZE;
        for (int tx = tx_left; tx <= tx_right; tx++) {
            if (world.IsSolid(tx, ty)) {
                state_.y         = static_cast<float>(ty * TILE_SIZE - TILE_SIZE);
                state_.vel_y     = 0.f;
                state_.on_ground = true;
                state_.dash_cooldown_ticks = 0;     // ricarica cooldown all'atterraggio
                state_.dash_ready          = true;  // ricarica la carica del dash
                state_.last_wall_jump_dir = 0;  // atterrato: può tornare a wall jumpare
                break;
            }
        }
    } else if (state_.vel_y < 0.f) {
        // Salto: controlla bordo superiore
        int ty = static_cast<int>(state_.y) / TILE_SIZE;
        for (int tx = tx_left; tx <= tx_right; tx++) {
            if (world.IsSolid(tx, ty)) {
                state_.y     = static_cast<float>((ty + 1) * TILE_SIZE);
                state_.vel_y = 0.f;
                break;
            }
        }
    }
}
