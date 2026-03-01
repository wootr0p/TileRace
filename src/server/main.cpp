// TileRace_Server — entry point standalone.
// Il loop del server è implementato in ServerLogic.cpp per essere condiviso
// con LocalServer (modalità offline, passo 20).

#include <cstdio>
#include <atomic>
#include <enet/enet.h>
#include "ServerLogic.h"
#include "Protocol.h"

int main() {
    if (enet_initialize() != 0) {
        fprintf(stderr, "[server] ERRORE: enet_initialize fallita\n");
        return 1;
    }

    printf("[server] premi Ctrl+C per uscire\n");

    // stop_flag rimane false per sempre in modalità standalone;
    // il processo termina con Ctrl+C (SIGINT).
    std::atomic<bool> stop{false};
    RunServer(SERVER_PORT, LOBBY_MAP_PATH, stop);

    enet_deinitialize();
    return 0;
}
