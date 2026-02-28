#pragma once
#include "PlayerState.h"
#include <cstdint>

// Numero massimo di giocatori in partita.
// Usato anche come limite ENet MAX_CLIENTS (non ha senso accettare più connessioni).
static constexpr int MAX_PLAYERS = 8;

// Snapshot dello stato di gioco inviato dal server a ogni tick.
// Contiene gli stati di tutti i player connessi; ogni PlayerState
// porta al suo interno last_processed_tick per la reconciliation lato client.
struct GameState {
    uint32_t    count                = 0;           // player attualmente in partita
    PlayerState players[MAX_PLAYERS] = {};          // stati autoritativi
};
