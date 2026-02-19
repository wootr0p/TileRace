#include "NetworkManager.h"
#include <iostream>

NetworkManager::NetworkManager()
    : host(nullptr)
    , peer(nullptr)
    , is_server(false)
    , is_connected(false)
    , last_error("")
{
}

NetworkManager::~NetworkManager() {
    if (peer) {
        enet_peer_disconnect(peer, 0);
        peer = nullptr;
    }
    if (host) {
        enet_host_destroy(host);
        host = nullptr;
    }
}

bool NetworkManager::StartServer(const char* address, enet_uint16 port, size_t max_clients) {
    if (enet_initialize() != 0) {
        SetError("Failed to initialize ENet");
        return false;
    }

    ENetAddress enet_address;
    enet_address.port = port;
    
    if (address == nullptr) {
        enet_address.host = ENET_HOST_ANY;
    } else {
        if (enet_address_set_host(&enet_address, address) != 0) {
            SetError("Failed to set host address");
            return false;
        }
    }

    host = enet_host_create(&enet_address, max_clients, 2, 0, 0);
    if (host == nullptr) {
        SetError("Failed to create server host");
        return false;
    }

    is_server = true;
    is_connected = true;
    return true;
}

bool NetworkManager::ConnectToServer(const char* host_address, enet_uint16 port) {
    if (enet_initialize() != 0) {
        SetError("Failed to initialize ENet");
        return false;
    }

    host = enet_host_create(nullptr, 1, 2, 0, 0);
    if (host == nullptr) {
        SetError("Failed to create client host");
        return false;
    }

    ENetAddress address;
    if (enet_address_set_host(&address, host_address) != 0) {
        SetError("Failed to set server address");
        enet_host_destroy(host);
        host = nullptr;
        return false;
    }
    address.port = port;

    peer = enet_host_connect(host, &address, 2, 0);
    if (peer == nullptr) {
        SetError("Failed to connect to server");
        enet_host_destroy(host);
        host = nullptr;
        return false;
    }

    // Wait for connection (with timeout)
    ENetEvent event;
    if (enet_host_service(host, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        is_connected = true;
        return true;
    }

    SetError("Connection timeout or failed");
    enet_peer_reset(peer);
    peer = nullptr;
    enet_host_destroy(host);
    host = nullptr;
    return false;
}

bool NetworkManager::Update(int timeout_ms) {
    if (!host) return false;

    ENetEvent event;
    int service_result = enet_host_service(host, &event, timeout_ms);

    if (service_result < 0) {
        SetError("Host service error");
        return false;
    }

    if (service_result == 0) {
        return true;
    }

    switch (event.type) {
    case ENET_EVENT_TYPE_CONNECT:
        std::cout << "Connection established" << std::endl;
        if (!is_server) {
            peer = event.peer;
        }
        is_connected = true;
        break;

    case ENET_EVENT_TYPE_RECEIVE:
        std::cout << "Received: " << (char*)event.packet->data << std::endl;
        enet_packet_destroy(event.packet);
        break;

    case ENET_EVENT_TYPE_DISCONNECT:
        std::cout << "Disconnected" << std::endl;
        is_connected = false;
        event.peer->data = nullptr;
        break;

    case ENET_EVENT_TYPE_NONE:
        break;
    }

    return true;
}

bool NetworkManager::SendPacket(const std::string& message, enet_uint8 channel, enet_uint32 flags) {
    if (!host || !peer) {
        SetError("Not connected");
        return false;
    }

    ENetPacket* packet = enet_packet_create(message.c_str(), message.length() + 1, flags);
    if (packet == nullptr) {
        SetError("Failed to create packet");
        return false;
    }

    if (enet_peer_send(peer, channel, packet) < 0) {
        SetError("Failed to send packet");
        enet_packet_destroy(packet);
        return false;
    }

    enet_host_flush(host);
    return true;
}
