// ServerLogic.cpp — loop ENet autoritativo condiviso tra TileRace_Server e LocalServer.
// Estratto da server/main.cpp (passo 20) per supportare la modalità offline.

#include "ServerLogic.h"
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <enet/enet.h>
#include "Protocol.h"
#include "World.h"
#include "Player.h"
#include "Physics.h"

void RunServer(uint16_t port, const char* map_path, std::atomic<bool>& stop_flag) {
    World world;
    if (!world.LoadFromFile(map_path)) {
        fprintf(stderr, "[server] ERRORE: mappa non trovata: %s\n", map_path);
        return;
    }
    printf("[server] mappa caricata (%d x %d tile)\n",
           world.GetWidth(), world.GetHeight());

    // Trova la posizione di spawn ('X') nella tilemap.
    float spawn_x = 0.f, spawn_y = 0.f;
    for (int ty = 0; ty < world.GetHeight(); ty++)
        for (int tx = 0; tx < world.GetWidth(); tx++)
            if (world.GetTile(tx, ty) == 'X') {
                spawn_x = static_cast<float>(tx * TILE_SIZE);
                spawn_y = static_cast<float>(ty * TILE_SIZE);
            }

    ENetAddress address{};
    address.host = ENET_HOST_ANY;
    address.port = port;

    ENetHost* server = enet_host_create(
        &address,
        static_cast<size_t>(MAX_CLIENTS),
        static_cast<size_t>(CHANNEL_COUNT),
        0, 0
    );
    if (!server) {
        fprintf(stderr,
                "[server] ERRORE: enet_host_create fallita (porta %u occupata?)\n", port);
        return;
    }
    printf("[server] in ascolto su UDP porta %u  (max %zu client)\n",
           port, static_cast<size_t>(MAX_CLIENTS));

    uint32_t next_player_id = 1;   // 0 = "sconosciuto" sul client
    std::unordered_map<ENetPeer*, Player> players;

    ENetEvent event;
    while (!stop_flag) {
        while (enet_host_service(server, &event, 16) > 0) {
            switch (event.type) {

            case ENET_EVENT_TYPE_CONNECT: {
                PlayerState ps{};
                ps.player_id = next_player_id++;
                ps.x = spawn_x;
                ps.y = spawn_y;
                Player pl{};
                pl.SetState(ps);
                players[event.peer] = pl;

                // Informa il client del suo player_id.
                PktWelcome welcome{};
                welcome.player_id = ps.player_id;
                ENetPacket* wlc = enet_packet_create(&welcome, sizeof(welcome),
                                                     ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(event.peer, CHANNEL_RELIABLE, wlc);
                printf("[server] CONNECT player_id=%u  %08x:%u\n",
                       ps.player_id,
                       event.peer->address.host,
                       event.peer->address.port);
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE: {
                const size_t len = event.packet->dataLength;
                if (len >= 1) {
                    const uint8_t type = event.packet->data[0];

                    // Input da simulare.
                    if (type == PKT_INPUT && len >= sizeof(PktInput)) {
                        PktInput pkt{};
                        std::memcpy(&pkt, event.packet->data, sizeof(PktInput));
                        auto it = players.find(event.peer);
                        if (it != players.end()) {
                            it->second.Simulate(pkt.frame, world);

                            // Costruisce il GameState con tutti i player e lo
                            // trasmette in broadcast a tutti i peer connessi.
                            PktGameState gs_pkt{};
                            gs_pkt.state.count = 0;
                            for (auto& [p, pl] : players) {
                                if (gs_pkt.state.count < static_cast<uint32_t>(MAX_PLAYERS))
                                    gs_pkt.state.players[gs_pkt.state.count++] = pl.GetState();
                            }
                            ENetPacket* bcast = enet_packet_create(
                                &gs_pkt, sizeof(gs_pkt), 0);
                            enet_host_broadcast(server, CHANNEL_RELIABLE, bcast);
                            enet_host_flush(server);
                        }
                    }
                    // Aggiornamento nome player.
                    else if (type == PKT_PLAYER_INFO && len >= sizeof(PktPlayerInfo)) {
                        PktPlayerInfo info{};
                        std::memcpy(&info, event.packet->data, sizeof(PktPlayerInfo));
                        auto it = players.find(event.peer);
                        if (it != players.end()) {
                            PlayerState s = it->second.GetState();
                            std::strncpy(s.name, info.name, sizeof(s.name) - 1);
                            s.name[sizeof(s.name) - 1] = '\0';
                            it->second.SetState(s);
                            printf("[server] PLAYER_INFO id=%u name='%s'\n",
                                   s.player_id, s.name);
                        }
                    }
                }
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("[server] DISCONNECT %08x:%u\n",
                       event.peer->address.host,
                       event.peer->address.port);
                players.erase(event.peer);
                break;

            default:
                break;
            }
        }
    }

    enet_host_destroy(server);
    printf("[server] fermato\n");
}
