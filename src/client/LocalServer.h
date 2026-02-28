#pragma once
#include <cstdint>
#include <atomic>
#include <thread>
#include <string>

// Avvia una istanza del game server nello stesso processo, in un thread background.
// Usato per la modalità offline (passo 20): il client si connette a 127.0.0.1 come
// se fosse online, senza dover avviare TileRace_Server separatamente.
class LocalServer {
public:
    LocalServer()  = default;
    ~LocalServer() { Stop(); }

    LocalServer(const LocalServer&)            = delete;
    LocalServer& operator=(const LocalServer&) = delete;

    // Avvia il server sulla porta indicata con la mappa indicata.
    // Ritorna dopo ~200 ms (attesa bind ENet).
    void Start(uint16_t port, const char* map_path);

    // Segnala lo stop e attende il termine del thread.
    void Stop();

    bool     IsRunning() const { return running_; }
    uint16_t GetPort()   const { return port_; }

private:
    std::thread       thread_;
    std::atomic<bool> stop_flag_{false};
    uint16_t          port_    = 0;
    bool              running_ = false;
};
