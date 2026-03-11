#pragma once
#include <cstdint>

// Plain-data player snapshot. Fully serialisable; shared between client and server.
struct PlayerState {
    // Position (pixels, top-left origin)
    float    x          = 0.f;
    float    y          = 0.f;

    // Velocity
    float    vel_x      = 0.f;   // transient impulse channel (e.g. wall-jump push)
    float    vel_y      = 0.f;
    float    move_vel_x = 0.f;   // inertia-smoothed horizontal speed (px/s)

    // Collision flags — updated each tick by MoveX / MoveY
    bool     on_ground     = false;
    bool     on_wall_left  = false;
    bool     on_wall_right = false;

    // Jump mechanics
    uint8_t  jump_buffer_ticks  = 0;  // > 0: buffered jump awaiting a valid surface
    uint8_t  coyote_ticks       = 0;  // > 0: grace ticks after leaving a ledge
    int8_t   last_wall_jump_dir = 0;  // prevents double-jump on same wall; reset on landing (-1/0/+1)

    // Dash
    uint8_t  dash_active_ticks   = 0;   // > 0: dash in progress (gravity + directional input suspended)
    uint8_t  dash_cooldown_ticks = 0;   // > 0: dash unavailable
    bool     dash_ready          = true; // false after air dash; reset on landing
    int8_t   last_dir            = 1;   // last non-zero horizontal dir; fallback dash target (-1/+1)
    float    dash_dir_x          = 0.f; // normalised current dash direction
    float    dash_dir_y          = 0.f;
    uint8_t  dash_jump_ticks     = 0;   // > 0: post-dash window where jump force is boosted

    // Magnet throw push (applied when a grabber dashes while carrying this player)
    uint8_t  launch_push_ticks   = 0;   // > 0: forced movement in launch_dir_*
    float    launch_dir_x        = 0.f; // normalised forced push direction
    float    launch_dir_y        = 0.f;

    // Identity / network
    uint32_t player_id           = 0;
    uint32_t last_processed_tick = 0;   // last tick acknowledged by server; drives client reconciliation

    // Display name (max 15 chars + null terminator)
    char     name[16]            = {};

    // Race timing — level_ticks freezes when finished = true
    uint32_t level_ticks         = 0;
    bool     finished            = false; // true after the player touches an exit tile 'E'

    // Kill tile respawn state
    uint8_t  kill_respawn_ticks  = 0;   // > 0: player is dead (touched 'K'); counts to 0 then respawns
    uint8_t  respawn_grace_ticks = 0;   // > 0: just respawned; input blocked; triggers Ready/Go! overlay on client

    // Checkpoint — updated server-side when the player touches a 'C' tile.
    // (0,0) means no checkpoint has been reached yet (fall back to level spawn).
    float    checkpoint_x = 0.f;
    float    checkpoint_y = 0.f;

    // Drawing trail — true while the player holds the draw button
    bool     drawing      = false;

    // Sprint — true while the player holds the sprint button
    bool     sprinting    = false;

    // Magnet — true while the player holds the magnet button
    bool     magneting    = false;

    // Grabbed — true while another player is carrying this player via magnet
    bool     grabbed      = false;
};