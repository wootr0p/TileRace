#include <SDL3/SDL.h>
#include <enet/enet.h>
#include "../common/Protocol.h"
#include <iostream>

int main(int argc, char** argv) {
    // Inizializzazioni
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
    if (enet_initialize() != 0) return 1;

    SDL_Window* window = SDL_CreateWindow("TileRace Client", 640, 480, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    // Connessione ENet
    ENetHost* client = enet_host_create(NULL, 1, 2, 0, 0);
    ENetAddress address;
    enet_address_set_host(&address, "localhost");
    address.port = 1234;
    ENetPeer* peer = enet_host_connect(client, &address, 2, 0);

    bool running = true;
    while (running) {
        // 1. SDL Event Loop
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) running = false;
            if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_SPACE) {
                // Invia PING quando premi SPAZIO
                NetworkMessage msg = { PACKET_PING, 100 };
                ENetPacket* p = enet_packet_create(&msg, sizeof(NetworkMessage), ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(peer, 0, p);
            }
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
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    enet_host_destroy(client);
    SDL_Quit();
    return 0;
}