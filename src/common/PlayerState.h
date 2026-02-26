#pragma once
#include <cstdint>

// =============================================================================
// PlayerState.h — Snapshot puro dello stato di un giocatore
// =============================================================================
//
// CONCETTO CHIAVE: "State vs. Entity"
//
// PlayerState è solo DATI. Non ha comportamento, non ha metodi.
// È la "fotografia" dello stato di un giocatore in un preciso tick.
//
// Perché questa separazione?
//   1. SERIALIZZAZIONE: possiamo mandare PlayerState direttamente sulla rete
//      (con attenzione all'endianness) senza dover "spacchettare" una classe.
//   2. RECONCILIATION: il client tiene una storia di PlayerState predetti;
//      può confrontare quello predetto al tick N con quello ricevuto dal server.
//   3. ROLLBACK: per fare rollback basta copiare un vecchio PlayerState.
//
// Challenge: se aggiungi dati al giocatore (es. health, score), vanno qui?
// Dipende: vanno qui SE devono essere simulati/replicati per la prediction.
// =============================================================================

struct PlayerState {
    uint32_t player_id  = 0;

    // Posizione nel mondo (unità logiche, NON pixel)
    float x   = 0.0f;
    float y   = 0.0f;

    // Velocità attuale
    float vel_x = 0.0f;
    float vel_y = 0.0f;

    // Stato di collisione
    bool on_ground     = false;
    bool on_wall_left  = false;   // tocca un muro a sinistra (per wall-jump)
    bool on_wall_right = false;   // tocca un muro a destra  (per wall-jump)

    // Ultima direzione orizzontale: -1 sinistra, 0 fermo, 1 destra
    // Usata da dash (quando non si tiene un tasto) e da wall-jump
    int8_t last_dir = 0;

    // Dash
    uint8_t dash_active_ticks   = 0;  // tick rimasti nel dash corrente
    uint8_t dash_cooldown_ticks = 0;  // tick rimasti al prossimo dash disponibile

    // CRUCIALE per la reconciliation: il client usa questo per sapere quali input
    // ha già "consumato" il server e quali sono ancora pendenti.
    uint32_t last_processed_tick = 0;
};
