#pragma once
// SoundPool — pool of N audio variants for the same effect.
// Picks a random variant and applies a slight pitch randomisation on every play.
// PlayAt() additionally maps world-space distance to volume attenuation
// and horizontal offset to stereo pan (L/R).
// No external dependencies beyond Raylib.
#include <raylib.h>
#include <vector>

class SoundPool {
public:
    // Load all N sound files whose paths are listed in [paths, paths+count).
    // Call once at startup (after InitAudioDevice).
    void Load(const char* const* paths, int count);

    // Unload all loaded sounds. Call before CloseAudioDevice.
    void Unload();

    // Play a random variant at full volume, centre pan.
    // pitch_variance: ± range around 1.0 (e.g. 0.07 → pitch in [0.93, 1.07]).
    void Play(float pitch_variance = 0.07f);

    // Play with 2-D spatial audio.
    //   source_x / source_y  : world-space pixel position of the emitter.
    //   listener_x / listener_y: world-space pixel position of the listener (camera target).
    //   max_dist             : distance (px) at which volume hits 0.
    void PlayAt(float source_x, float source_y,
                float listener_x, float listener_y,
                float max_dist       = 1200.f,
                float pitch_variance = 0.07f);

private:
    std::vector<Sound> sounds_;

    // Returns the next sound to play (round-robin through the pool so concurrent
    // calls on different remote players don't stomp each other's pitch/pan state).
    Sound& NextVoice();
    int next_idx_ = 0;
};
