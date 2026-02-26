#pragma once
#include <deque>
#include <string>
#include <enet/enet.h>
#include "../common/GameState.h"
#include "../common/Player.h"
#include "../common/World.h"
#include "../common/Protocol.h"
#include "InputHandler.h"
#include "Renderer.h"
#include "AudioManager.h"

// =============================================================================
// GameClient.h — Il client con Client-Side Prediction + Server Reconciliation
// =============================================================================
//
// CONCETTO FONDAMENTALE: il client è OTTIMISTA ma non ARROGANTE.
//
//   OTTIMISTA:    applica il proprio input immediatamente, senza aspettare il
//                 server, per dare feedback istantaneo al giocatore.
//   NON ARROGANTE: sa che il server può avere ragione su ciò che è successo
//                  davvero, e in quel caso si corregge silenziosamente.
//
// Il ciclo di prediction/reconciliation funziona così:
//
//   1. CLIENT TICK N:
//      a. Campiona l'input (InputFrame)
//      b. Simula il proprio giocatore localmente (prediction)
//      c. Salva (tick, input, stato_predetto) nel buffer
//      d. Invia l'input al server
//
//   2. SERVER TICK N (elaborato dopo latency):
//      a. Riceve l'input
//      b. Simula autoritativamente
//      c. Invia GameState con last_processed_tick = N
//
//   3. CLIENT riceve GameState:
//      a. Confronta lo stato server con lo stato predetto al tick N
//      b. Se UGUALE → tutto ok, scarta dal buffer i tick ≤ N
//      c. Se DIVERSO → RECONCILIATION:
//         - Imposta lo stato del giocatore locale = stato del server
//         - Re-simula TUTTI gli input nel buffer dopo il tick N
//         - Aggiorna lo stato locale con il risultato
//
// Risultato: il giocatore non vede mai un "rubber band" evidente,
// e il server rimane l'autorità suprema.
// =============================================================================

class GameClient {
public:
    GameClient();
    ~GameClient();

    bool Init(const std::string& server_host, uint16_t port,
              const std::string& map_path, uint32_t session_token = 0);

    // Blocca il thread nel game loop (fixed timestep + rendering senza cap)
    void Run();

private:
    // ---- Game Loop ----
    void ProcessNetworkEvents();
    void Tick();                        // Fixed-rate: 60Hz
    void Render(float alpha);           // Variable-rate: massimo FPS

    // ---- Prediction ----
    // Applica l'input localmente e salva nel buffer
    void PredictStep(const InputFrame& input);

    // ---- Reconciliation ----
    // Ricevuto uno stato autoritativo dal server: verifica e correggi
    void Reconcile(const GameState& server_state);

    // ---- Rete ----
    void OnConnected(ENetPeer* peer);
    void OnDisconnected();
    void OnGameStateReceived(const PktGameState& pkt);
    void OnAudioEventReceived(const PktAudioEvent& pkt);
    void OnPlayerAssigned(const PktPlayerAssigned& pkt);

    void SendInput(const InputFrame& frame);

    // ---- Dati ----

    // Il giocatore locale — simulato dalla prediction
    Player local_player_;

    // Lo stato di tutti gli altri giocatori (aggiornato dal server)
    GameState world_state_;
    // Snapshot del frame precedente per interpolare visivamente tutti i player
    GameState prev_world_state_;

    World world_;

    // -------------------------
    // BUFFER DI PREDICTION
    // -------------------------
    // Ogni voce = ciò che abbiamo inviato al server e non ancora confermato.
    // Necessario per la reconciliation: se il server dice che il tick 42
    // era sbagliato, dobbiamo ri-simulare dal tick 42 in poi.
    struct PredictionEntry {
        uint32_t    tick;
        InputFrame  input;
        PlayerState state_after; // Stato del giocatore DOPO aver applicato questo input
    };

    std::deque<PredictionEntry> prediction_buffer_;

    // Quante voci massimo teniamo in memoria?
    // 128 tick = ~2 secondi a 60Hz. Abbastanza per compensare latenze ragionevoli.
    static constexpr size_t MAX_BUFFER_SIZE = 128;

    // -------------------------
    // Stato connessione
    // -------------------------
    uint32_t local_player_id_ = 0;
    uint32_t client_tick_     = 0;
    uint32_t session_token_   = 0;  // inviato in PktConnectRequest
    bool     connected_       = false;

    ENetHost* host_        = nullptr;
    ENetPeer* server_peer_ = nullptr;

    // Posizione del giocatore locale al tick PRECEDENTE.
    // Usata per interpolare la camera tra un tick e il successivo,
    // eliminando il jitter visivo causato dal fixed timestep.
    float prev_x_ = 0.0f;
    float prev_y_ = 0.0f;

    // ---- Sottosistemi ----
    InputHandler input_handler_;
    Renderer     renderer_;
    AudioManager audio_;
};
