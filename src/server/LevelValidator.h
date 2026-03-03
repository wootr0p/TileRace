#pragma once
// SRP: validates generated levels by simulating an AI agent through them.
// Uses the real Player::Simulate physics to BFS over reachable tile positions.
// Returns true if a path from spawn ('X') to end ('E') exists.
// No ENet or Raylib dependency.

#include "World.h"

class LevelValidator {
public:
    // Returns true if the level has a viable path from spawn to any 'E' tile
    // using the full game physics (jump, dash, wall-jump, dash-jump).
    static bool Validate(const World& world);
};
