#include <enet/enet.h>
#include <iostream>
#include "GameServer.h"

int main(int /*argc*/, char** /*argv*/) {
    if (enet_initialize() != 0) {
        std::cerr << "[Server] ERRORE: impossibile inizializzare ENet.\n";
        return 1;
    }

    GameServer server;
    if (!server.Init(1234, 8, "assets/levels/level_01.txt")) {
        enet_deinitialize();
        return 1;
    }

    server.Run();

    enet_deinitialize();
    return 0;
}