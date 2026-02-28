#include "LocalServer.h"
#include "ServerLogic.h"   // da src/server/
#include <chrono>
#include <thread>

void LocalServer::Start(uint16_t port, const char* map_path) {
    if (running_) Stop();

    port_      = port;
    stop_flag_ = false;
    running_   = true;

    // Copia la stringa prima di spostarla nel thread.
    std::string map_copy(map_path);
    thread_ = std::thread([this, port, map_copy]() {
        RunServer(port, map_copy.c_str(), stop_flag_);
    });

    // Attende che il server abbia il tempo di fare il bind sulla porta.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void LocalServer::Stop() {
    if (!running_) return;
    stop_flag_ = true;
    if (thread_.joinable()) thread_.join();
    running_ = false;
}
