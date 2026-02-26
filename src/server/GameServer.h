#pragma once
#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <enet/enet.h>
#include "../common/GameState.h"
#include "../common/Player.h"
#include "../common/World.h"
#include "../common/Protocol.h"

// =============================================================================
// GameServer.h — L'autorità suprema del gioco
// =============================================================================
//
// RESPONSABILITÀ:
//   1. Mantenere lo stato "vero" del gioco (nessuno può contraddirlo)
//   2. Ricevere InputFrame dai client
//   3. Simulare la fisica per ogni giocatore (usando lo stesso Player::Simulate
//      che usa il client per la prediction)
//   4. Trasmettere GameState a tutti i client ogni tick
//   5. Gestire connessioni/disconnessioni
//
// PRINCIPIO: il server NON si fida del client. Riceve solo InputFrame.
// Mai posizioni. Mai velocità. Il server calcola tutto da solo.
//
// ARCHITETTURA: GameServer è SEPARATO da main.cpp per testabilità.
// main.cpp fa solo inizializzazione e chiama server.Run().
// =============================================================================

class GameServer {
public:
    GameServer();
    ~GameServer();

    // Inizializza ENet e carica il mondo. Ritorna false se fallisce.
    // local_only=true → bind su 127.0.0.1 (solo loopback, non raggiungibile da rete)
    bool Init(uint16_t port, int max_clients, const std::string& map_path,
              bool local_only = false);

    // Imposta il token di sessione (0 = nessuna autenticazione).
    // Chiamare PRIMA di Run(). Usato da LocalServer per isolare la partita.
    void SetSessionToken(uint32_t token) { session_token_ = token; }

    // Blocca il thread corrente nel game loop (fixed timestep 60Hz)
    void Run();

    // Segnala al loop di fermarsi. Thread-safe: chiamabile da un altro thread.
    void Stop();

private:
    // ---- Game Loop ----

    // Processa tutti gli eventi di rete in coda (non-blocking)
    void ProcessNetworkEvents();

    // Simula UN tick di gioco (applica tutti gli input bufferizzati)
    void Tick();

    // Invia lo stato autoritativo a tutti i client connessi
    void BroadcastGameState();

    // ---- Handlers ----

    void OnClientConnected(ENetPeer* peer);
    void OnClientDisconnected(ENetPeer* peer);
    void OnConnectRequest(ENetPeer* peer, const PktConnectRequest& pkt);
    void OnInputReceived(ENetPeer* peer, const PktInput& pkt);

    // ---- Helpers ----

    // Assegna un ID univoco al nuovo giocatore e risponde al client
    uint32_t AssignPlayerId(ENetPeer* peer);

    // Invia un pacchetto a un singolo peer
    void SendTo(ENetPeer* peer, const void* data, size_t size, bool reliable = true);

    // Invia un pacchetto a tutti i peer connessi
    void Broadcast(const void* data, size_t size, bool reliable = true);

    // ---- Dati ----

    ENetHost* host_ = nullptr;

    // Peer connessi a livello ENet ma in attesa di autenticazione
    // (non ancora giocatori riconosciuti)
    std::unordered_set<ENetPeer*> pending_auth_;

    // Token di sessione: il client deve inviarlo in PktConnectRequest.
    // 0 = disabilitato (server pubblico standalone).
    uint32_t session_token_ = 0;

    // peer_id → Player (l'ID del peer ENet viene usato come chiave temporanea)
    // player_id è assegnato da noi, peer_id è assegnato da ENet
    std::unordered_map<uint32_t, Player>     players_;      // player_id → Player
    std::unordered_map<ENetPeer*, uint32_t>  peer_to_id_;   // peer ENet → player_id

    // Buffer degli input ricevuti in questo tick, prima che vengano processati
    // Perché un buffer? Perché i pacchetti arrivano in qualsiasi momento
    // tra un tick e l'altro, ma devono essere processati TUTTI insieme al tick.
    std::unordered_map<uint32_t, InputFrame> pending_inputs_; // player_id → ultimo InputFrame

    World    world_;
    uint32_t server_tick_     = 0;
    uint32_t next_player_id_  = 1;  // 0 è riservato come "nessuno"
    std::atomic<bool> running_{ false };
};
