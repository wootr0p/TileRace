#pragma once
#include "PlayerState.h"
#include "InputFrame.h"

// Deterministic player simulation. Owns a PlayerState and updates it each tick.
// Used identically on client (prediction + reconciliation) and server (authority).
class World;

class Player {
public:
    void SetState(const PlayerState& s) { state_ = s; }
    const PlayerState& GetState() const { return state_; }

    // Main update: apply one InputFrame to the current state.
    // Runs all mechanics (movement, coyote, jump, dash, gravity, collision) in fixed order.
    void Simulate(const InputFrame& frame, const World& world);

    // Low-level helpers — exposed for unit tests and split-step internal use.
    void MoveX(float input_dx, const World& world);
    void MoveY(float dt, const World& world);
    void RequestJump();
    void CutJump();    // call when jump key is released while the player is still rising
    void RequestDash(float dx, float dy);
    void SteerDash(float dx, float dy);

    // Reset non-serialised transient state (call on level change).
    void ResetTransient() { prev_jump_held_ = false; }

private:
    PlayerState state_;
    bool        prev_jump_held_ = false;

    void ResolveCollisionsX(const World& world, float dx);
    void ResolveCollisionsY(const World& world);
};
