#include <raylib.h>
#include <enet/enet.h>
#include "../common/Protocol.h"
#include <iostream>

int main(int argc, char** argv) {
    // Inizializzazioni
    if (enet_initialize() != 0) return 1;

    InitWindow(640, 480, "TileRace Client");
    SetTargetFPS(60);

    // Connessione ENet
    ENetHost* client = enet_host_create(NULL, 1, 2, 0, 0);
    ENetAddress address;
    enet_address_set_host(&address, "localhost");
    address.port = 1234;
    ENetPeer* peer = enet_host_connect(client, &address, 2, 0);

    while (!WindowShouldClose()) {
        // 1. Input raylib
        if (IsKeyPressed(KEY_SPACE)) {
            // Invia PING quando premi SPAZIO
            NetworkMessage msg = { PACKET_PING, 100 };
            ENetPacket* p = enet_packet_create(&msg, sizeof(NetworkMessage), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(peer, 0, p);
        }

        // 2. ENet Event Loop
        ENetEvent netEv;
        while (enet_host_service(client, &netEv, 0) > 0) {
            if (netEv.type == ENET_EVENT_TYPE_RECEIVE) {
                NetworkMessage* msg = (NetworkMessage*)netEv.packet->data;
                if (msg->type == PACKET_PONG) {
                    std::cout << "Ricevuto PONG dal server: " << msg->value << std::endl;
                }
                enet_packet_destroy(netEv.packet);
            }
        }

        // 3. Rendering
        BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawText("Premi SPAZIO per inviare PING", 20, 20, 20, RAYWHITE);
        DrawText("Chiudi la finestra per uscire", 20, 50, 20, LIGHTGRAY);
        EndDrawing();
    }

    enet_host_destroy(client);
    enet_deinitialize();
    CloseWindow();
    return 0;
}