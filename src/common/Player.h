#pragma once
#include "PlayerState.h"
#include "InputFrame.h"

// Classe Player: possiede uno PlayerState e lo aggiorna.
// Evoluzione prevista:
//   passo 4-7  --> MoveX/MoveY + meccaniche di salto   — COMPLETATO
//   passo 9    --> Simulate(InputFrame, World)           — COMPLETATO
class World;   // forward declaration; World.h incluso solo in Player.cpp

class Player {
public:
    void SetState(const PlayerState& s) { state_ = s; }
    const PlayerState& GetState() const { return state_; }

    // Punto unico di aggiornamento deterministico (passo 9).
    // Legge l'InputFrame, esegue tutte le meccaniche (salto, dash, movimento,
    // gravità, collisioni) nell'ordine corretto e aggiorna state_.
    // Identica su client e server: stessi input + stesso stato = stesso risultato.
    void Simulate(const InputFrame& frame, const World& world);

    // --- Metodi di basso livello (usati internamente + esposti per test) ---

    // Muove orizzontalmente di input_dx (già moltiplicato per dt).
    void MoveX(float input_dx, const World& world);

    // Applica coyote time, salto, gravità, collisioni Y.
    void MoveY(float dt, const World& world);

    // Registra l'intenzione di saltare: setta jump_buffer_ticks.
    void RequestJump();

    // Taglia il salto quando SPAZIO viene rilasciato mentre il player sale.
    void CutJump();

    // Avvia il dash nella direzione (dx, dy) — normalizzata internamente.
    void RequestDash(float dx, float dy);

    // Aggiorna la direzione del dash in corso (sterzata live).
    void SteerDash(float dx, float dy);

private:
    PlayerState state_;
    bool        prev_jump_held_ = false;  // per rilevare il rilascio del tasto salto

    void ResolveCollisionsX(const World& world, float dx);
    void ResolveCollisionsY(const World& world);
};
