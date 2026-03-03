// VisualEffects.cpp — implementazione delle strutture dati client-only.
// Separato da Renderer.cpp perché non richiede API di disegno Raylib,
// solo GetRandomValue che è nell'API di utilità.

#include "VisualEffects.h"
#include <cmath>

void DeathParticles::Spawn(float cx, float cy, Color c) {
    col = c;
    for (int i = 0; i < MAX; i++) {
        const float angle = static_cast<float>(i) / static_cast<float>(MAX) * 6.2832f
                            + GetRandomValue(0, 314) * 0.01f;
        const float speed = 100.f + static_cast<float>(GetRandomValue(0, 320));
        parts[i] = { cx, cy, cosf(angle) * speed, sinf(angle) * speed, 1.f };
    }
}

void DeathParticles::Update(float dt) {
    for (auto& p : parts) {
        if (p.life <= 0.f) continue;
        p.x  += p.vx * dt;
        p.y  += p.vy * dt;
        p.vy += 400.f * dt;
        p.life -= 1.5f * dt;
    }
}

bool DeathParticles::Active() const {
    for (const auto& p : parts) if (p.life > 0.f) return true;
    return false;
}
