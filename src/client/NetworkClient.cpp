// NetworkClient.cpp — unico file che include <enet/enet.h> nel client.
// Se si sostituisce ENet con un'altra libreria, si riscrive solo questo file.

#include "NetworkClient.h"
#include "Protocol.h"   // CHANNEL_COUNT, CHANNEL_RELIABLE
#include <enet/enet.h>
#include <cstdio>
#include <cstring>

// Alias per i cast (evita verbosità ripetuta)
#define HOST  static_cast<ENetHost*>(host_)
#define PEER  static_cast<ENetPeer*>(peer_)

NetworkClient::NetworkClient()  = default;

NetworkClient::~NetworkClient() {
    Disconnect();
}

bool NetworkClient::Connect(const char* ip, uint16_t port, int attempts) {
    host_ = enet_host_create(nullptr, 1, CHANNEL_COUNT, 0, 0);
    if (!host_) {
        fprintf(stderr, "[net] enet_host_create fallita\n");
        return false;
    }

    ENetAddress addr{};
    enet_address_set_host(&addr, ip);
    addr.port = port;

    peer_ = enet_host_connect(HOST, &addr, CHANNEL_COUNT, 0);
    if (!peer_) {
        fprintf(stderr, "[net] enet_host_connect fallita\n");
        enet_host_destroy(HOST);
        host_ = nullptr;
        return false;
    }

    printf("[net] connessione a %s:%u in corso...\n", ip, port);
    for (int i = 0; i < attempts; ++i) {
        ENetEvent ev;
        if (enet_host_service(HOST, &ev, 50) > 0 &&
            ev.type == ENET_EVENT_TYPE_CONNECT) {
            printf("[net] connesso\n");
            return true;
        }
    }

    fprintf(stderr, "[net] server non raggiungibile\n");
    enet_peer_reset(PEER);
    enet_host_destroy(HOST);
    peer_ = nullptr;
    host_ = nullptr;
    return false;
}

void NetworkClient::Disconnect() {
    if (peer_) {
        if (PEER->state == ENET_PEER_STATE_CONNECTED)
            enet_peer_disconnect_now(PEER, 0);
        peer_ = nullptr;
    }
    if (host_) {
        enet_host_destroy(HOST);
        host_ = nullptr;
    }
}

void NetworkClient::SendReliable(const void* data, size_t size) {
    if (!peer_) return;
    ENetPacket* pkt = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(PEER, CHANNEL_RELIABLE, pkt);
}

void NetworkClient::SendUnreliable(const void* data, size_t size) {
    if (!peer_) return;
    ENetPacket* pkt = enet_packet_create(data, size, 0);
    enet_peer_send(PEER, CHANNEL_RELIABLE, pkt);
}

NetEvent NetworkClient::Poll() {
    NetEvent result;
    if (!host_) return result;

    ENetEvent ev;
    const int ret = enet_host_service(HOST, &ev, 0);
    if (ret <= 0) return result;

    if (ev.type == ENET_EVENT_TYPE_RECEIVE && ev.packet) {
        result.type = NetEventType::Packet;
        result.data.assign(ev.packet->data,
                           ev.packet->data + ev.packet->dataLength);
        enet_packet_destroy(ev.packet);
    } else if (ev.type == ENET_EVENT_TYPE_DISCONNECT) {
        result.type           = NetEventType::Disconnected;
        result.disconnect_data = ev.data;
        peer_ = nullptr;  // il peer è invalido dopo DISCONNECT
    }
    return result;
}

uint32_t NetworkClient::GetRTT() const {
    if (!peer_) return 0;
    return PEER->roundTripTime;
}

uint32_t NetworkClient::GetJitter() const {
    if (!peer_) return 0;
    return PEER->roundTripTimeVariance;
}

uint32_t NetworkClient::GetLoss() const {
    if (!peer_) return 0;
    return PEER->packetLoss * 100u / ENET_PEER_PACKET_LOSS_SCALE;
}

#undef HOST
#undef PEER
