#pragma once
#include "../common/GameState.h"
#include "../common/World.h"

// =============================================================================
// Renderer.h — Tutto ciò che riguarda il disegno su schermo
// =============================================================================
//
// PRINCIPIO FONDAMENTALE: il Renderer è un "viewer" passivo.
// Riceve GameState e World come dati di SOLA LETTURA e li disegna.
// NON modifica niente. NON prende decisioni di gioco.
//
// Perché questa separazione è cruciale?
//   - La logica (common) può girare in headless mode (es. sui test server)
//   - Il rendering non influenza mai la simulazione
//   - Puoi sostituire Raylib con un'altra libreria grafica senza toccare
//     una riga di codice in /common o /server
//
// INTERPOLAZIONE VISIVA:
//   Per rendere il movimento fluido nonostante il fixed timestep,
//   il renderer interpola tra l'ultimo stato e quello precedente
//   usando un fattore alpha (0.0 → 1.0) basato sull'accumulator del game loop.
// =============================================================================

class Renderer {
public:
    // Inizializza la finestra Raylib
    bool Init(int width, int height, const char* title);

    // Chiude la finestra e libera le risorse
    void Shutdown();

    // Disegna un frame completo.
    //   cam_x, cam_y: posizione camera già interpolata dal chiamante (lerp prev/curr * alpha).
    //                 Disaccoppia il Renderer dalla logica di interpolazione.
    void Draw(const GameState& state, const World& world,
              uint32_t local_player_id, float cam_x, float cam_y) const;

    [[nodiscard]] bool ShouldClose() const;

private:
    void DrawWorld(const World& world) const;
    void DrawPlayers(const GameState& state, uint32_t local_player_id) const;
    void DrawHUD(const GameState& state, uint32_t local_player_id) const;

    int screen_w_ = 0;
    int screen_h_ = 0;
};
