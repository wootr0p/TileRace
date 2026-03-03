#include "SoundPool.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>  // rand()

// ---------------------------------------------------------------------------
// Load / Unload
// ---------------------------------------------------------------------------
void SoundPool::Load(const char* const* paths, int count) {
    sounds_.clear();
    sounds_.reserve(count);
    for (int i = 0; i < count; ++i) {
        Sound s = LoadSound(paths[i]);
        if (s.stream.buffer != nullptr)
            sounds_.push_back(s);
    }
    next_idx_ = 0;
}

void SoundPool::Unload() {
    for (Sound& s : sounds_)
        UnloadSound(s);
    sounds_.clear();
}

// ---------------------------------------------------------------------------
// Internal voice picker (round-robin to avoid concurrent pitch/pan stomping)
// ---------------------------------------------------------------------------
Sound& SoundPool::NextVoice() {
    if (sounds_.empty()) {
        static Sound dummy{};
        return dummy;
    }
    // Random pick biased away from repeating the same variant back-to-back.
    if (sounds_.size() == 1) { next_idx_ = 0; return sounds_[0]; }
    // Pick randomly among all indices except the previous one.
    int r = std::rand() % static_cast<int>(sounds_.size() - 1);
    if (r >= next_idx_) ++r;
    next_idx_ = r;
    return sounds_[r];
}

// ---------------------------------------------------------------------------
// Play (no spatialization)
// ---------------------------------------------------------------------------
void SoundPool::Play(float pitch_variance) {
    if (sounds_.empty()) return;
    Sound& s = NextVoice();
    // Random pitch in [1-var, 1+var].
    const float pitch = 1.f + (static_cast<float>(std::rand() % 2001 - 1000) / 1000.f) * pitch_variance;
    SetSoundPitch(s, pitch);
    SetSoundPan(s, 0.5f);
    SetSoundVolume(s, 1.f);
    PlaySound(s);
}

// ---------------------------------------------------------------------------
// PlayAt (2-D spatial audio)
// ---------------------------------------------------------------------------
void SoundPool::PlayAt(float source_x, float source_y,
                       float listener_x, float listener_y,
                       float max_dist, float pitch_variance) {
    if (sounds_.empty()) return;

    const float dx   = source_x - listener_x;
    const float dy   = source_y - listener_y;
    const float dist = std::sqrtf(dx * dx + dy * dy);

    // Cull sounds beyond max_dist (saves processing audio for far-off players).
    if (dist >= max_dist) return;

    // Linear volume falloff: 1 at distance 0, 0 at max_dist.
    const float volume = 1.f - dist / max_dist;

    // Pan: 0=left, 0.5=centre, 1=right. Full left at -max_dist/2, full right at +max_dist/2.
    const float pan_range = max_dist * 0.5f;
    const float pan = std::clamp(0.5f + dx / pan_range, 0.f, 1.f);

    Sound& s = NextVoice();
    const float pitch = 1.f + (static_cast<float>(std::rand() % 2001 - 1000) / 1000.f) * pitch_variance;
    SetSoundPitch(s, pitch);
    SetSoundPan(s, pan);
    SetSoundVolume(s, volume);
    PlaySound(s);
}
