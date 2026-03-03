#pragma once
// ENet abstraction for the client side.
// <enet/enet.h> is NOT included here — the ENet dependency is fully contained in NetworkClient.cpp.
// Replacing ENet requires rewriting only that .cpp file.
#include <cstdint>
#include <cstddef>
#include <vector>

enum class NetEventType { None, Disconnected, Packet };

struct NetEvent {
    NetEventType         type            = NetEventType::None;
    std::vector<uint8_t> data;            // valid when type == Packet
    uint32_t             disconnect_data = 0; // valid when type == Disconnected
};

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    NetworkClient(const NetworkClient&)            = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    // Blocking connect (max `attempts` × 50 ms). Returns false on timeout.
    bool Connect(const char* ip, uint16_t port, int attempts = 60);

    void Disconnect();

    bool IsConnected() const { return peer_ != nullptr; }

    void SendReliable  (const void* data, size_t size);
    void SendUnreliable(const void* data, size_t size);

    // Returns the next queued event. Call in a loop until type == None.
    // Packet bytes are already copied; the underlying ENet packet is destroyed internally.
    NetEvent Poll();

    // Network stats — all zero when not connected.
    uint32_t GetRTT()    const;
    uint32_t GetJitter() const;
    uint32_t GetLoss()   const;

private:
    void* host_ = nullptr;  // ENetHost* — opaque in this header
    void* peer_ = nullptr;  // ENetPeer* — opaque in this header
};
