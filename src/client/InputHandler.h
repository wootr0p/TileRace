#pragma once
#include "../common/InputFrame.h"

// =============================================================================
// InputHandler.h — Cattura l'input fisico (Raylib) e produce InputFrame
// =============================================================================
//
// SEPARAZIONE DELLE RESPONSABILITÀ:
//   InputHandler sa come leggere i tasti (dipendenza da Raylib).
//   Il resto del codice NON sa da dove viene l'input.
//
// Vantaggio diretto: se un giorno vuoi registrare/riprodurre input per
// il testing (replay system), basta creare un MockInputHandler che legge
// da file invece che dalla tastiera. Il GameClient non cambia.
//
// QUESTO FILE HA LA SOLA DIPENDENZA DA RAYLIB nel lato input.
// Tutto il resto del client usa solo InputFrame.
// =============================================================================

class InputHandler {
public:
    // Chiama UNA VOLTA per render frame, DOPO BeginDrawing (cioè dopo Render()).
    // Cattura IsKeyPressed prima che il prossimo BeginDrawing lo resetti.
    // I tasti one-shot (salto) vengono messi in un flag "sticky" che persiste
    // finché un tick fisso non lo consuma tramite Sample().
    void Poll();

    // Chiama UNA VOLTA per tick fisso.
    // Legge lo stato dei tasti continui (movimento) e consuma i flag sticky (salto).
    [[nodiscard]] InputFrame Sample(uint32_t current_tick);

private:
    // Flag sticky: impostato da Poll(), azzerato da Sample().
    // Sopravvive ai frame di rendering senza tick.
    bool jump_pending_ = false;
    bool dash_pending_ = false;
};
