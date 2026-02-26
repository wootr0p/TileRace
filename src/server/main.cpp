#include <iostream>
#include <enet/enet.h>
#include "../common/Protocol.h" // Includiamo la logica condivisa

int main(int argc, char** argv) {
    if (enet_initialize() != 0) return 1;

    ENetAddress address;
    address.host = ENET_HOST_ANY; // Ascolta da chiunque
    address.port = 1234;          // Porta del gioco

    ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);
    if (!server) return 1;

    std::cout << "Server avviato sulla porta 1234..." << std::endl;

    ENetEvent event;
    while (true) {
        // Aspetta un evento per 10ms
        while (enet_host_service(server, &event, 10) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE: {
                    // Castiamo i dati ricevuti nel nostro tipo condiviso
                    NetworkMessage* msg = (NetworkMessage*)event.packet->data;
                    if (msg->type == PACKET_PING) {
                        std::cout << "Ricevuto PING: " << msg->value << std::endl;

                        // Rispondiamo con un PONG
                        NetworkMessage response = { PACKET_PONG, msg->value + 1 };
                        ENetPacket* packet = enet_packet_create(&response, sizeof(NetworkMessage), ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(event.peer, 0, packet);
                    }
                    enet_packet_destroy(event.packet);
                    break;
                }
            }
        }
    }
    enet_host_destroy(server);
    enet_deinitialize();
    return 0;
}