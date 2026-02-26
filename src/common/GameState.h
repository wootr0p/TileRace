#pragma once
#include <cstdint>
#include "PlayerState.h"

// =============================================================================
// GameState.h — Snapshot completo del mondo per un singolo tick
// =============================================================================
//
// GameState è la "verità" del mondo in un preciso momento.
// Il server la genera ogni tick e la manda ai client.
// Il client ne mantiene UNA SOLA versione "corrente" (che include la prediction
// locale), ma ne archivia tante versioni predette nel buffer della reconciliation.
//
// Nota: MAX_PLAYERS è fisso per evitare allocazioni dinamiche nel critical path
// della simulazione e per semplificare la serializzazione di rete.
// =============================================================================

static constexpr int MAX_PLAYERS = 8;

struct GameState {
    uint32_t    server_tick   = 0;   // Il tick del server che ha generato questo stato
    uint8_t     player_count  = 0;
    PlayerState players[MAX_PLAYERS];

    // Cerca lo stato di un giocatore per ID — ritorna nullptr se non trovato
    [[nodiscard]] const PlayerState* FindPlayer(uint32_t id) const {
        for (int i = 0; i < player_count; ++i) {
            if (players[i].player_id == id) return &players[i];
        }
        return nullptr;
    }

    // Versione mutabile
    [[nodiscard]] PlayerState* FindPlayer(uint32_t id) {
        for (int i = 0; i < player_count; ++i) {
            if (players[i].player_id == id) return &players[i];
        }
        return nullptr;
    }
};
