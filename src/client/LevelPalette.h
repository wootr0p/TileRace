#pragma once
// LevelPalette — per-level world colour theme derived from a level number.
//
// Only the world/environment colours change; player colours stay fixed so every
// player always recognises their own character regardless of the current level.
//
// Generation algorithm:
//   base_hue = level_num * GOLDEN_ANGLE_DEG  (mod 360)
//              → 8 levels spread maximally across the hue wheel
//   Background : same hue, S=0.65, L=0.08   (almost black — guarantees player contrast)
//   Wall       : same hue, S=0.45, L=0.42   (ΔL≈34% vs BG → always readable)
//   Exit       : base_hue +120°, S=0.65, L=0.38  (triadic — always distinct from wall)
//   Kill       : base_hue +180°, S=0.85, L=0.50  (complementary — always distinct)
//   Checkpoint : base_hue +100°, S=0.70, L=0.50  (near-triadic — distinct from kill/wall)
//   Spawn      : transparent (invisible in-game, kept for completeness)
//
// level_num == 0  →  returns the lobby default palette (original Colors.h values).
//
// Header-only; no .cpp required.  Depends only on <raylib.h> and <cmath>.
#include <raylib.h>
#include <cmath>

struct LevelPalette {
    Color bg;
    Color wall;
    Color exit_tile;
    Color kill_tile;
    Color checkpoint;
    Color spawn;  // typically transparent / invisible

    // Default constructor = lobby palette (original Colors.h values).
    // Used for file-based levels and the lobby map.
    LevelPalette()
        : bg        {  8,  12,  40, 255}
        , wall      { 60, 100, 180, 255}
        , exit_tile { 14, 140, 124, 255}
        , kill_tile {220,  50,  50, 255}
        , checkpoint{ 50, 230,  80, 255}
        , spawn     {106, 111,  50,   0}
    {}
};

namespace detail {

// h in [0, 360), s and l in [0, 1]  →  Color {r, g, b, alpha}
inline Color HslToColor(float h, float s, float l, uint8_t alpha = 255) {
    h = std::fmodf(h, 360.f);
    if (h < 0.f) h += 360.f;
    const float c  = (1.f - std::fabsf(2.f * l - 1.f)) * s;
    const float x  = c * (1.f - std::fabsf(std::fmodf(h / 60.f, 2.f) - 1.f));
    const float m  = l - c * 0.5f;
    float r = 0.f, g = 0.f, b = 0.f;
    if      (h <  60.f) { r = c; g = x; }
    else if (h < 120.f) { r = x; g = c; }
    else if (h < 180.f) {        g = c; b = x; }
    else if (h < 240.f) {        g = x; b = c; }
    else if (h < 300.f) { r = x;        b = c; }
    else                { r = c;        b = x; }
    return { static_cast<uint8_t>((r + m) * 255.f),
             static_cast<uint8_t>((g + m) * 255.f),
             static_cast<uint8_t>((b + m) * 255.f),
             alpha };
}

} // namespace detail

// Returns the palette for a given level number (1-based).
// level_num == 0 returns the default lobby palette (same as LevelPalette{}).
inline LevelPalette MakeLevelPalette(uint8_t level_num) {
    if (level_num == 0) return LevelPalette{};

    // Golden-angle distribution: maximally separates up to ~16 distinct hues.
    static constexpr float GOLDEN_DEG = 137.508f;
    const float h = std::fmodf(level_num * GOLDEN_DEG, 360.f);

    LevelPalette p;
    p.bg         = detail::HslToColor(h,          0.65f, 0.08f);
    p.wall       = detail::HslToColor(h,          0.45f, 0.42f);
    // Exit and checkpoint share the same visual role ("reach me") — same color.
    p.exit_tile  = detail::HslToColor(h + 100.f,  0.70f, 0.50f);
    // Kill tile stays fixed red regardless of hue — universal danger signal.
    p.kill_tile  = {220, 50, 50, 255};
    p.checkpoint = detail::HslToColor(h + 100.f,  0.70f, 0.50f);
    p.spawn      = {106, 111, 50, 0};  // always transparent (not rendered)
    return p;
}
