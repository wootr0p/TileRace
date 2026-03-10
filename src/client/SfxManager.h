#pragma once
// SfxManager — owns all SoundPools and one-shot sounds for the game.
// Loads all assets once during GameSession construction; unloads on destruction.
// Play*()   methods: local player (full volume, centre pan).
// Play*At() methods: remote players (2-D spatialized).
#include "SoundPool.h"
#include <raylib.h>

class SfxManager {
public:
    SfxManager();
    ~SfxManager();

    // No copy.
    SfxManager(const SfxManager&)            = delete;
    SfxManager& operator=(const SfxManager&) = delete;

    void SetMuted(bool muted) { muted_ = muted; }
    bool IsMuted()      const { return muted_; }

    // --- Movement sounds ---
    // Local
    void PlayJump();
    void PlayWallJump();
    void PlayDash();
    // Remote (spatialized)
    void PlayJumpAt    (float sx, float sy, float lx, float ly);
    void PlayWallJumpAt(float sx, float sy, float lx, float ly);
    void PlayDashAt    (float sx, float sy, float lx, float ly);

    // --- Death sounds ---
    void PlayDeath();
    void PlayDeathAt(float sx, float sy, float lx, float ly);

    // --- Checkpoint ---
    void PlayCheckpoint();
    void PlayCheckpointAt(float sx, float sy, float lx, float ly);

    // --- Level complete ---
    void PlayLevelEnd();
    void PlayLevelEndAt(float sx, float sy, float lx, float ly);

    // --- UI / level events (local only, no spatialization) ---
    void PlayReady();
    void PlayGo();
    void PlayGrabOn();
    void PlayGrabOff();
    void PlayGrabOnAt(float sx, float sy, float lx, float ly);
    void PlayGrabOffAt(float sx, float sy, float lx, float ly);

private:
    SoundPool jump_pool_;      // 3 variants
    SoundPool wall_jump_pool_; // 3 variants
    SoundPool dash_pool_;      // 3 variants
    SoundPool death_pool_;     // 3 variants
    SoundPool checkpoint_pool_;// 1 variant
    Sound     ready_sound_    = {};
    Sound     go_sound_       = {};
    Sound     level_end_sound_= {};
    Sound     grab_on_sound_  = {};
    Sound     grab_off_sound_ = {};
    bool      muted_ = false;
};
