#pragma once
#include <thread>
#include <memory>
#include <string>
#include <cstdint>

// Forward declaration — non vogliamo includere GameServer.h qui per
// non portare le dipendenze ENet nel codice del client.
class GameServer;

// =============================================================================
// LocalServer.h — Server in-process per la modalità offline
// =============================================================================
//
// In modalità offline non serve un processo separato: basta far girare
// GameServer in un thread background all'interno del processo del client.
// Il client si connette poi a "localhost" esattamente come farebbe online.
//
// Perché questo approccio invece di chiamare la logica del server direttamente?
//   - Il codice del gioco rimane identico: il client non sa se il server
//     è locale o remoto. Zero casi speciali nel GameClient.
//   - Il server conserva la sua autorità e il suo fixed timestep indipendente.
//   - In futuro si può "staccare" il server senza toccare il client.
// =============================================================================

class LocalServer {
public:
    LocalServer();
    ~LocalServer();

    // Avvia il server in un thread background.
    // Ritorna true se il server ha completato l'inizializzazione con successo.
    // Blocca per al massimo ~200ms in attesa che il server sia pronto.
    bool Start(uint16_t port, const std::string& map_path);

    // Segnala al server di fermarsi e aspetta che il thread termini.
    void Stop();

    [[nodiscard]] bool     IsRunning()       const;
    // Token segreto generato all'avvio: il client lo invia in PktConnectRequest.
    // Un altro processo che non conosce questo valore viene rifiutato.
    [[nodiscard]] uint32_t GetSessionToken() const { return session_token_; }

private:
    std::unique_ptr<GameServer> server_;
    std::thread                 thread_;
    uint32_t                    session_token_ = 0;
};
