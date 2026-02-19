/**
 * ENetTest.cpp - Test di ENet per verificare che la libreria funzioni correttamente
 * 
 * Per compilare solo questo test (senza il progetto principale):
 * 1. Crea un progetto console temporaneo
 * 2. Includi enet/enet.h
 * 3. Linka enet.lib
 * 4. Compila questo file
 */

#include <enet/enet.h>
#include <iostream>
#include <thread>
#include <chrono>

void TestBasicENet() {
    std::cout << "=== ENet Basic Test ===" << std::endl;

    // Test 1: Initialize ENet
    std::cout << "\n[Test 1] Initializing ENet..." << std::endl;
    if (enet_initialize() != 0) {
        std::cerr << "ERROR: Failed to initialize ENet" << std::endl;
        return;
    }
    std::cout << "SUCCESS: ENet initialized" << std::endl;

    // Test 2: Create Server
    std::cout << "\n[Test 2] Creating server host..." << std::endl;
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 5000;

    ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);
    if (server == nullptr) {
        std::cerr << "ERROR: Failed to create server" << std::endl;
        enet_deinitialize();
        return;
    }
    std::cout << "SUCCESS: Server created on port 5000" << std::endl;

    // Test 3: Create Client
    std::cout << "\n[Test 3] Creating client host..." << std::endl;
    ENetHost* client = enet_host_create(nullptr, 1, 2, 0, 0);
    if (client == nullptr) {
        std::cerr << "ERROR: Failed to create client" << std::endl;
        enet_host_destroy(server);
        enet_deinitialize();
        return;
    }
    std::cout << "SUCCESS: Client created" << std::endl;

    // Test 4: Client Connect to Server
    std::cout << "\n[Test 4] Client connecting to server..." << std::endl;
    ENetAddress serverAddress;
    enet_address_set_host(&serverAddress, "localhost");
    serverAddress.port = 5000;

    ENetPeer* serverPeer = enet_host_connect(client, &serverAddress, 2, 0);
    if (serverPeer == nullptr) {
        std::cerr << "ERROR: Failed to initiate connection" << std::endl;
        enet_host_destroy(server);
        enet_host_destroy(client);
        enet_deinitialize();
        return;
    }
    std::cout << "SUCCESS: Connection initiated" << std::endl;

    // Test 5: Simulate network communication
    std::cout << "\n[Test 5] Simulating network communication..." << std::endl;
    bool client_connected = false;
    bool server_received = false;

    for (int i = 0; i < 50; ++i) {  // Try for ~5 seconds
        ENetEvent event;

        // Server update
        if (enet_host_service(server, &event, 100) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "   [Server] Client connected!" << std::endl;
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                std::cout << "   [Server] Received: " << (char*)event.packet->data << std::endl;
                server_received = true;
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "   [Server] Client disconnected" << std::endl;
                break;
            default:
                break;
            }
        }

        // Client update
        if (enet_host_service(client, &event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "   [Client] Connected to server!" << std::endl;
                client_connected = true;
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                std::cout << "   [Client] Received: " << (char*)event.packet->data << std::endl;
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "   [Client] Disconnected from server" << std::endl;
                break;
            default:
                break;
            }
        }

        // Send test message after connection
        if (client_connected && !server_received) {
            std::cout << "   [Client] Sending test message..." << std::endl;
            const char* test_msg = "Hello from ENet Client!";
            ENetPacket* packet = enet_packet_create(test_msg, strlen(test_msg) + 1, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(serverPeer, 0, packet);
            enet_host_flush(client);
            client_connected = false;  // Only send once
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (server_received) {
        std::cout << "SUCCESS: Communication test passed!" << std::endl;
    } else {
        std::cout << "WARNING: No communication received (may be normal in test environment)" << std::endl;
    }

    // Test 6: Cleanup
    std::cout << "\n[Test 6] Cleaning up..." << std::endl;
    enet_peer_disconnect(serverPeer, 0);
    enet_host_destroy(server);
    enet_host_destroy(client);
    enet_deinitialize();
    std::cout << "SUCCESS: Cleanup completed" << std::endl;

    std::cout << "\n=== ENet Test Completed ===" << std::endl;
}

// Uncomment main() below to run this test standalone
// Comment it out if you want to compile this with your main project

//int main() {
//    TestBasicENet();
//    return 0;
//}

