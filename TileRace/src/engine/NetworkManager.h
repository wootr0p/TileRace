#pragma once
#include <enet/enet.h>
#include <string>

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    // Server functions
    bool StartServer(const char* host, enet_uint16 port, size_t max_clients);
    
    // Client functions
    bool ConnectToServer(const char* host, enet_uint16 port);
    
    // Network update
    bool Update(int timeout_ms = 0);
    
    // Send functions
    bool SendPacket(const std::string& message, enet_uint8 channel = 0, enet_uint32 flags = ENET_PACKET_FLAG_RELIABLE);
    
    // Query functions
    bool IsConnected() const { return is_connected; }
    bool IsServer() const { return is_server; }
    std::string GetLastError() const { return last_error; }

private:
    ENetHost* host;
    ENetPeer* peer;
    bool is_server;
    bool is_connected;
    std::string last_error;

    void SetError(const std::string& error) { last_error = error; }
};
