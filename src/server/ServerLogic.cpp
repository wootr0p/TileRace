// ServerLogic.cpp — loop ENet autoritativo (TileRace_Server + LocalServer).
// Responsabilità: creare l'host ENet, servire gli eventi, delegare a ServerSession.
// Tutta la logica di sessione vive in ServerSession.

#include "ServerLogic.h"
#include "ServerSession.h"
#include "Protocol.h"
#include <cstdio>
#include <cstring>
#include <enet/enet.h>

void RunServer(uint16_t port, const char* map_path, std::atomic<bool>& stop_flag) {
    printf("[server] TileRace v%s  (protocol %u)\n", GAME_VERSION, PROTOCOL_VERSION);

    // Estrai il numero di livello iniziale dal nome del file (es. "level_02.txt" → 2).
    int initial_level = 1;
    {
        const char* p = std::strstr(map_path, "level_");
        if (p) std::sscanf(p + 6, "%d", &initial_level);
    }

    ServerSession session(map_path, initial_level);
    if (!session.IsReady()) {
        fprintf(stderr, "[server] ERRORE: mappa non trovata: %s\n", map_path);
        return;
    }

    ENetAddress address{};
    address.host = ENET_HOST_ANY;
    address.port = port;
    ENetHost* server = enet_host_create(
        &address,
        static_cast<size_t>(MAX_CLIENTS),
        static_cast<size_t>(CHANNEL_COUNT),
        0, 0);
    if (!server) {
        fprintf(stderr,
                "[server] ERRORE: enet_host_create fallita (porta %u occupata?)\n", port);
        return;
    }
    printf("[server] in ascolto su UDP porta %u  (max %zu client)\n",
           port, static_cast<size_t>(MAX_CLIENTS));

    ENetEvent event;
    while (!stop_flag) {
        // Servi tutti gli eventi disponibili in questo ciclo.
        // Se un handler restituisce true (cambio livello avvenuto), smetti
        // di processare altri eventi questo ciclo per evitare stato inconsistente.
        bool level_changed = false;
        while (!level_changed && enet_host_service(server, &event, 16) > 0) {
            switch (event.type) {

            case ENET_EVENT_TYPE_CONNECT:
                session.OnConnect(server, event.peer);
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                if (event.packet->dataLength >= 1)
                    level_changed = session.OnReceive(
                        server, event.peer,
                        event.packet->data,
                        event.packet->dataLength);
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                level_changed = session.OnDisconnect(server, event.peer);
                break;

            default:
                break;
            }
        }

        // Controllo timer (results timeout): eseguito una volta per ciclo.
        if (!level_changed)
            session.CheckTimers(server);
    }

    enet_host_destroy(server);
    printf("[server] fermato\n");
}
