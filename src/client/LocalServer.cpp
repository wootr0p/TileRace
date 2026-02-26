#include "LocalServer.h"
#include "../server/GameServer.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <random>

LocalServer::LocalServer()  = default;

LocalServer::~LocalServer() {
    Stop();
}

bool LocalServer::Start(uint16_t port, const std::string& map_path) {
    // Genera un token di sessione casuale a 32-bit non-zero.
    // Un altro processo che tenta di connettersi a localhost non conosce questo valore.
    std::random_device rd;
    std::uniform_int_distribution<uint32_t> dist(1u, 0xFFFFFFFFu);
    session_token_ = dist(rd);

    server_ = std::make_unique<GameServer>();

    if (!server_->Init(port, 4, map_path, /*local_only=*/true)) {
        std::cerr << "[LocalServer] Impossibile inizializzare il server locale.\n";
        server_.reset();
        return false;
    }

    server_->SetSessionToken(session_token_);

    // Avvia il game loop del server in un thread dedicato.
    // Il thread esegue Run() che blocca con il proprio fixed-timestep loop.
    thread_ = std::thread([this]() {
        server_->Run();
    });

    // Dai al server ~150ms per aprire la porta ENet prima che il client tenti
    // la connessione. In localhost la latenza di bind è trascurabile, ma
    // lo scheduling dei thread non è deterministico.
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    return true;
}

void LocalServer::Stop() {
    if (server_) {
        server_->Stop();
    }
    if (thread_.joinable()) {
        thread_.join();
    }
    server_.reset();
}

bool LocalServer::IsRunning() const {
    return server_ != nullptr;
}
