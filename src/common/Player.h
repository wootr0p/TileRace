#pragma once
#include "PlayerState.h"
#include "InputFrame.h"

// Forward declaration — Player NON include World.h direttamente per evitare
// dipendenze circolari. Riceve il World come parametro nelle funzioni.
class World;

// =============================================================================
// Player.h — Entità giocatore con logica di simulazione
// =============================================================================
//
// DISTINZIONE FONDAMENTALE:
//   PlayerState = DATI (in PlayerState.h) — cosa è il giocatore
//   Player      = ENTITÀ (qui)            — cosa sa fare il giocatore
//
// Player possiede un PlayerState e sa come simularlo.
// Questa separazione permette di:
//   1. Copiare/confrontare stati senza copiare comportamento
//   2. Usare PlayerState come snapshot di rete
//   3. Ricreare un Player da un PlayerState (per il rollback)
//
// DOMANDA: perché non mettere Simulate() direttamente in PlayerState?
// Risposta: perché PlayerState deve essere POD-like (semplice, serializzabile).
// Aggiungere metodi lo complicherebbe senza beneficio.
// =============================================================================

class Player {
public:
    explicit Player(uint32_t id);

    // Simula UN tick deterministico: applica input + fisica + collisioni
    void Simulate(const InputFrame& input, const World& world);

    void SetState(const PlayerState& state);

    [[nodiscard]] const PlayerState& GetState() const { return state_; }
    [[nodiscard]] PlayerState&       GetState()       { return state_; }

private:
    void ResolveCollisionsX(const World& world);
    void ResolveCollisionsY(const World& world);

    PlayerState state_;

    static constexpr float PLAYER_W = 32.0f;
    static constexpr float PLAYER_H = 32.0f;
    // Inset verticale per il check X: evita falsi positivi con pavimento/soffitto
    static constexpr float X_INSET  = 2.0f;
};
