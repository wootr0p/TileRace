#pragma once
#include <string>
#include "../common/Protocol.h"

// =============================================================================
// AudioManager.h — Sistema audio event-based
// =============================================================================
//
// ARCHITETTURA EVENT-BASED:
//   Il server decide QUANDO riprodurre un suono (es. al momento del salto).
//   Il client decide COME riprodurlo (quale file, quale volume, pan stereo).
//
// Perché non fare decidere il client autonomamente?
//   1. SINCRONIZZAZIONE: se il client suona il jump al frame di input locale,
//      ma il server scarta l'input (es. double-jump non valido), si sente
//      un suono fantasma.
//   2. COERENZA MULTIPLAYER: con l'event-based, TUTTI i client sentono lo stesso
//      evento allo stesso tempo (compensando il latency).
//
// Background musicale: il client può gestirlo autonomamente (non dipende
// dalla logica di gioco), ma gli SFX di gameplay vanno attraverso il server.
// =============================================================================

class AudioManager {
public:
    // Carica tutti gli asset audio. Da chiamare una volta all'avvio.
    bool Init(const std::string& assets_path);

    // Libera le risorse Raylib audio
    void Shutdown();

    // Processa un evento audio arrivato dal server
    void PlayEvent(const PktAudioEvent& event);

    // Aggiorna lo stato interno (es. stream musicali) — da chiamare ogni frame
    void Update();

private:
    // In futuro: std::unordered_map<AudioEventId, Sound> sounds_;
    bool initialized_ = false;
};
