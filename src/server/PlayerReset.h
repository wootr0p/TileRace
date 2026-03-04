#pragma once
// Header-only helper that removes duplicate 15-line reset blocks from ServerSession
// (kill tile, restart, level-change events all converge here).
#include "PlayerState.h"

// Reset all movement fields and place the player at spawn coordinates.
//   with_kill = true  → kill_respawn_ticks = 60  (death animation, ~1 s wait before control)
//   with_kill = false → respawn_grace_ticks = 25  (triggers Ready/Go! overlay on the client, ~0.4 s)
// Also clears checkpoint_x/checkpoint_y and resets level_ticks (full restart semantics).
inline PlayerState SpawnReset(PlayerState s, float sx, float sy, bool with_kill) {
    s.x             = sx;
    s.y             = sy;
    s.vel_x         = 0.f;
    s.vel_y         = 0.f;
    s.move_vel_x    = 0.f;
    s.on_ground     = false;
    s.on_wall_left  = false;
    s.on_wall_right = false;
    s.jump_buffer_ticks  = 0;
    s.coyote_ticks       = 0;
    s.last_wall_jump_dir = 0;
    s.dash_active_ticks  = 0;
    s.dash_cooldown_ticks= 0;
    s.dash_ready         = true;
    s.dash_dir_x         = 0.f;
    s.dash_dir_y         = 0.f;
    s.dash_jump_ticks    = 0;
    s.level_ticks        = 0;
    s.finished           = false;
    s.checkpoint_x       = 0.f;  // clear checkpoint on full spawn reset
    s.checkpoint_y       = 0.f;
    s.drawing            = false;
    s.sprinting          = false;
    s.magneting          = false;
    s.grabbed            = false;
    if (with_kill) {
        s.kill_respawn_ticks  = 60;
        s.respawn_grace_ticks = 0;
    } else {
        s.kill_respawn_ticks  = 0;
        s.respawn_grace_ticks = 25;
    }
    return s;
}

// Respawn the player at a checkpoint position.
// Unlike SpawnReset, level_ticks keeps accumulating (time penalty only from downtime)
// and the checkpoint itself is preserved.
// with_kill = true  → kill_respawn_ticks = 60 (used by automatic kill-tile death)
// with_kill = false → respawn_grace_ticks = 25 (used by manual restart-at-checkpoint)
inline PlayerState CheckpointReset(PlayerState s, float cx, float cy, bool with_kill) {
    s.x             = cx;
    s.y             = cy;
    s.vel_x         = 0.f;
    s.vel_y         = 0.f;
    s.move_vel_x    = 0.f;
    s.on_ground     = false;
    s.on_wall_left  = false;
    s.on_wall_right = false;
    s.jump_buffer_ticks  = 0;
    s.coyote_ticks       = 0;
    s.last_wall_jump_dir = 0;
    s.dash_active_ticks  = 0;
    s.dash_cooldown_ticks= 0;
    s.dash_ready         = true;
    s.dash_dir_x         = 0.f;
    s.dash_dir_y         = 0.f;
    s.dash_jump_ticks    = 0;
    // level_ticks intentionally NOT reset — time keeps running
    s.finished           = false;
    s.drawing            = false;
    s.sprinting          = false;
    s.magneting          = false;
    s.grabbed            = false;
    // checkpoint coordinates preserved
    if (with_kill) {
        s.kill_respawn_ticks  = 60;
        s.respawn_grace_ticks = 0;
    } else {
        s.kill_respawn_ticks  = 0;
        s.respawn_grace_ticks = 25;
    }
    return s;
}
