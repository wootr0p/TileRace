#pragma once
#include <cstdint>
#include <atomic>
#include <thread>
#include <string>
#include "GameMode.h"

// Runs a game server instance in a background thread within the same process.
// Used for offline mode: the client connects to 127.0.0.1 via ENet as normal
// without needing a separate TileRace_Server executable.
class LocalServer {
public:
    LocalServer()  = default;
    ~LocalServer() { Stop(); }

    LocalServer(const LocalServer&)            = delete;
    LocalServer& operator=(const LocalServer&) = delete;

    // Start the server on the given port. Blocks ~200 ms waiting for the ENet bind.
    // When skip_lobby is true the lobby is skipped and level 1 is generated immediately.
    // initial_mode sets the starting game mode (RACE for offline).
    void Start(uint16_t port, const char* map_path, bool skip_lobby = false,
               GameMode initial_mode = GameMode::COOP);

    // Signal stop and join the background thread.
    void Stop();

    bool     IsRunning() const { return running_; }
    uint16_t GetPort()   const { return port_; }

private:
    std::thread       thread_;
    std::atomic<bool> stop_flag_{false};
    uint16_t          port_    = 0;
    bool              running_ = false;
};
