#include <enet/enet.h>
#include <raylib.h>
#include <iostream>
#include <memory>
#include "GameClient.h"
#include "MainMenu.h"
#include "LocalServer.h"

int main(int /*argc*/, char** /*argv*/) {
    if (enet_initialize() != 0) {
        std::cerr << "[Client] ERRORE: impossibile inizializzare ENet.\n";
        return 1;
    }

    // Apriamo la finestra qui, una volta sola.
    // Sia il MainMenu che il Renderer la riusano (Renderer::Init controlla IsWindowReady).
    InitWindow(960, 540, "TileRace");
    SetTargetFPS(144);

    // ---- Menu ----
    MainMenu menu;
    MenuResult choice = menu.Show();

    if (choice.choice == MenuChoice::Quit) {
        CloseWindow();
        enet_deinitialize();
        return 0;
    }

    // ---- Modalità Offline: avvia server locale ----
    std::unique_ptr<LocalServer> local_server;
    if (choice.choice == MenuChoice::Offline) {
        local_server = std::make_unique<LocalServer>();
        if (!local_server->Start(1234, "assets/levels/level_01.txt")) {
            std::cerr << "[Client] ERRORE: impossibile avviare il server locale.\n";
            CloseWindow();
            enet_deinitialize();
            return 1;
        }
        choice.server_ip = "localhost";
        choice.port      = 1234;
    }

    // ---- Avvia il client ----
    GameClient client;
    if (!client.Init(choice.server_ip, choice.port, "assets/levels/level_01.txt",
                     local_server ? local_server->GetSessionToken() : 0u)) {
        CloseWindow();
        enet_deinitialize();
        return 1;
    }

    client.Run();

    // ---- Cleanup ----
    // Stop del server locale (se offline): segnala e aspetta il thread
    if (local_server) {
        local_server->Stop();
    }

    CloseWindow();
    enet_deinitialize();
    return 0;
}