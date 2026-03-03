#include "SfxManager.h"
#include <cmath>

namespace {
    static const char* JUMP_PATHS[] = {
        "assets/sfx/jump_01.wav",
        "assets/sfx/jump_02.wav",
        "assets/sfx/jump_03.wav",
    };
    static const char* WALL_JUMP_PATHS[] = {
        "assets/sfx/wall_jump_01.wav",
        "assets/sfx/wall_jump_02.wav",
        "assets/sfx/wall_jump_03.wav",
    };
    static const char* DASH_PATHS[] = {
        "assets/sfx/dash_01.wav",
        "assets/sfx/dash_02.wav",
        "assets/sfx/dash_03.wav",
    };
    static const char* DEATH_PATHS[] = {
        "assets/sfx/death_01.wav",
        "assets/sfx/death_02.wav",
        "assets/sfx/death_03.wav",
    };
    static const char* CHECKPOINT_PATHS[] = {
        "assets/sfx/checkpoint.wav",
    };
    static constexpr int   JUMP_COUNT        = 3;
    static constexpr int   WALL_JUMP_COUNT   = 3;
    static constexpr int   DASH_COUNT        = 3;
    static constexpr int   DEATH_COUNT       = 3;
    static constexpr int   CHECKPOINT_COUNT  = 1;
    static constexpr float MAX_DIST          = 1400.f;  // px — beyond this, remote sounds are silent
}

SfxManager::SfxManager() {
    jump_pool_.Load(JUMP_PATHS,           JUMP_COUNT);
    wall_jump_pool_.Load(WALL_JUMP_PATHS, WALL_JUMP_COUNT);
    dash_pool_.Load(DASH_PATHS,           DASH_COUNT);
    death_pool_.Load(DEATH_PATHS,         DEATH_COUNT);
    checkpoint_pool_.Load(CHECKPOINT_PATHS, CHECKPOINT_COUNT);
    ready_sound_     = LoadSound("assets/sfx/ready.wav");
    go_sound_        = LoadSound("assets/sfx/go.wav");
    level_end_sound_ = LoadSound("assets/sfx/level_end.wav");
}

SfxManager::~SfxManager() {
    jump_pool_.Unload();
    wall_jump_pool_.Unload();
    dash_pool_.Unload();
    death_pool_.Unload();
    checkpoint_pool_.Unload();
    UnloadSound(ready_sound_);
    UnloadSound(go_sound_);
    UnloadSound(level_end_sound_);
}

// ---------------------------------------------------------------------------
// Jump
// ---------------------------------------------------------------------------
void SfxManager::PlayJump() {
    if (!muted_) jump_pool_.Play();
}
void SfxManager::PlayJumpAt(float sx, float sy, float lx, float ly) {
    if (!muted_) jump_pool_.PlayAt(sx, sy, lx, ly, MAX_DIST);
}

// ---------------------------------------------------------------------------
// Wall jump
// ---------------------------------------------------------------------------
void SfxManager::PlayWallJump() {
    if (!muted_) wall_jump_pool_.Play();
}
void SfxManager::PlayWallJumpAt(float sx, float sy, float lx, float ly) {
    if (!muted_) wall_jump_pool_.PlayAt(sx, sy, lx, ly, MAX_DIST);
}

// ---------------------------------------------------------------------------
// Dash
// ---------------------------------------------------------------------------
void SfxManager::PlayDash() {
    if (!muted_) dash_pool_.Play();
}
void SfxManager::PlayDashAt(float sx, float sy, float lx, float ly) {
    if (!muted_) dash_pool_.PlayAt(sx, sy, lx, ly, MAX_DIST);
}

// ---------------------------------------------------------------------------
// Death
// ---------------------------------------------------------------------------
void SfxManager::PlayDeath() {
    if (!muted_) death_pool_.Play();
}
void SfxManager::PlayDeathAt(float sx, float sy, float lx, float ly) {
    if (!muted_) death_pool_.PlayAt(sx, sy, lx, ly, MAX_DIST);
}

// ---------------------------------------------------------------------------
// Checkpoint
// ---------------------------------------------------------------------------
void SfxManager::PlayCheckpoint() {
    if (!muted_) checkpoint_pool_.Play(0.04f);
}
void SfxManager::PlayCheckpointAt(float sx, float sy, float lx, float ly) {
    if (!muted_) checkpoint_pool_.PlayAt(sx, sy, lx, ly, MAX_DIST, 0.04f);
}

// ---------------------------------------------------------------------------
// Level complete
// ---------------------------------------------------------------------------
void SfxManager::PlayLevelEnd() {
    if (!muted_) PlaySound(level_end_sound_);
}
void SfxManager::PlayLevelEndAt(float sx, float sy, float lx, float ly) {
    // level_end is a single Sound (not a pool); play it spatialized via a manual volume/pan approach.
    // For simplicity we compute distance and play at reduced volume if far away.
    if (muted_) return;
    const float dx   = sx - lx;
    const float dy   = sy - ly;
    const float dist = sqrtf(dx * dx + dy * dy);
    constexpr float MAX_D = 1400.f;
    if (dist >= MAX_D) return;
    const float vol = 1.f - dist / MAX_D;
    const float pan = std::max(0.f, std::min(1.f, 0.5f + dx / (MAX_D * 0.5f)));
    SetSoundVolume(level_end_sound_, vol);
    SetSoundPan(level_end_sound_, pan);
    PlaySound(level_end_sound_);
    // Restore defaults so the local (non-spatialized) call still works
    SetSoundVolume(level_end_sound_, 1.f);
    SetSoundPan(level_end_sound_, 0.5f);
}

// ---------------------------------------------------------------------------
// UI / level events (local only)
// ---------------------------------------------------------------------------
void SfxManager::PlayReady() {
    if (!muted_) PlaySound(ready_sound_);
}
void SfxManager::PlayGo() {
    if (!muted_) PlaySound(go_sound_);
}
