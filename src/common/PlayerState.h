#pragma once
#include <cstdint>

// Dati POD del player — nessuna logica, solo stato serializzabile.
// Aggiornato incrementalmente passo per passo:
//   passo 4  → x, y, player_id
//   passo 5  → vel_x, vel_y, on_ground
//   passo 6  → on_wall_left, on_wall_right, jump_buffer_ticks, coyote_ticks
//   passo 9  → last_processed_tick
//   passo 17 → dash_active_ticks, dash_cooldown_ticks, last_dir
struct PlayerState {
    // posizione
    float    x         = 0.f;
    float    y         = 0.f;
    // velocità
    float    vel_x     = 0.f;   // impulso orizzontale (es. wall jump)
    float    vel_y     = 0.f;
    float    move_vel_x = 0.f;  // velocità orizzontale con inerzia (px/s)
    // flag collisione
    bool     on_ground     = false;
    bool     on_wall_left  = false;
    bool     on_wall_right = false;
    // meccaniche di salto avanzate (passo 6 ext)
    uint8_t  jump_buffer_ticks  = 0;  // > 0: salto in attesa di atterraggio/muro
    uint8_t  coyote_ticks       = 0;  // > 0: grazia dopo aver lasciato un bordo
    int8_t   last_wall_jump_dir = 0;  // -1=sinistra, 0=nessuno, 1=destra
                                      // impedisce di saltare due volte sullo stesso muro;
                                      // resettato quando si tocca terra
    // dash (passo 17)
    uint8_t  dash_active_ticks   = 0;   // > 0: dash in corso (sospende gravità e input)
    uint8_t  dash_cooldown_ticks = 0;   // > 0: cooldown, dash non disponibile
    bool     dash_ready          = true; // false dopo aver dashes in aria; reset all'atterraggio
    int8_t   last_dir            = 1;   // ultima dir orizzontale non-zero (-1/+1); fallback dash
    float    dash_dir_x          = 0.f; // componente X normalizzata della direzione di dash
    float    dash_dir_y          = 0.f; // componente Y normalizzata della direzione di dash
    // identificazione
    uint32_t player_id           = 0;
    uint32_t last_processed_tick = 0;   // ultimo frame.tick processato (usato per reconciliation)
    // nome visualizzato sopra il player (max 15 char + null)
    char     name[16]            = {};  // passo 19
};
