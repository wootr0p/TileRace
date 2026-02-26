#include "GameServer.h"
#include "../common/Physics.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

GameServer::GameServer()  = default;
GameServer::~GameServer() {
    if (host_) {
        enet_host_destroy(host_);
        host_ = nullptr;
    }
}

bool GameServer::Init(uint16_t port, int max_clients, const std::string& map_path,
                      bool local_only) {
    if (!world_.LoadFromFile(map_path)) {
        std::cerr << "[Server] ERRORE: impossibile caricare la mappa: " << map_path << "\n";
        return false;
    }

    ENetAddress address;
    if (local_only) {
        // Bind solo su loopback: connessioni da altre macchine rifiutate dal SO
        enet_address_set_host(&address, "127.0.0.1");
    } else {
        address.host = ENET_HOST_ANY;
    }
    address.port = port;

    host_ = enet_host_create(&address, static_cast<size_t>(max_clients), 2, 0, 0);
    if (!host_) {
        std::cerr << "[Server] ERRORE: impossibile creare l'host ENet sulla porta " << port << "\n";
        return false;
    }

    std::cout << "[Server] Avviato sulla porta " << port
              << " (max " << max_clients << " client)\n";
    return true;
}

void GameServer::Run() {
    running_.store(true);

    // Fixed timestep loop
    // Usiamo std::chrono per avere precisione sub-millisecondo.
    //
    // NOTA: questo è un "fixed timestep" classico.
    // Il server dorme quanto basta per rispettare 60Hz.
    // In produzione si userebbe un loop più sofisticato con "spiral of death" protection.

    using Clock     = std::chrono::steady_clock;
    using Duration  = std::chrono::duration<double>;

    const Duration tick_duration(Physics::FIXED_DT);
    auto next_tick_time = Clock::now() + tick_duration;

    while (running_) {
        ProcessNetworkEvents();
        Tick();
        BroadcastGameState();

        ++server_tick_;

        // Dormi fino al prossimo tick
        std::this_thread::sleep_until(next_tick_time);
        next_tick_time += tick_duration;
    }
}

void GameServer::Stop() {
    running_.store(false);
}

// =============================================================================
// NETWORK
// =============================================================================

void GameServer::ProcessNetworkEvents() {
    ENetEvent event;
    while (enet_host_service(host_, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                OnClientConnected(event.peer);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                OnClientDisconnected(event.peer);
                break;

            case ENET_EVENT_TYPE_RECEIVE: {
                if (event.packet->dataLength < sizeof(PacketHeader)) {
                    enet_packet_destroy(event.packet);
                    break;
                }
                PacketHeader header;
                std::memcpy(&header, event.packet->data, sizeof(header));

                if (header.type == PACKET_CONNECT_REQUEST &&
                    event.packet->dataLength >= sizeof(PktConnectRequest)) {
                    PktConnectRequest pkt;
                    std::memcpy(&pkt, event.packet->data, sizeof(pkt));
                    OnConnectRequest(event.peer, pkt);
                } else if (header.type == PACKET_INPUT &&
                    event.packet->dataLength >= sizeof(PktInput)) {
                    PktInput pkt;
                    std::memcpy(&pkt, event.packet->data, sizeof(pkt));
                    OnInputReceived(event.peer, pkt);
                }

                enet_packet_destroy(event.packet);
                break;
            }
            default: break;
        }
    }
}

void GameServer::OnClientConnected(ENetPeer* peer) {
    // Peer connesso a livello ENet: mettilo in attesa di autenticazione.
    // Non assegnare ancora un player_id: aspettiamo PktConnectRequest con il token.
    pending_auth_.insert(peer);
    std::cout << "[Server] Peer connesso, in attesa di autenticazione.\n";
}

void GameServer::OnConnectRequest(ENetPeer* peer, const PktConnectRequest& pkt) {
    // Verifica token solo se session_token_ != 0 (modalità locale)
    if (session_token_ != 0 && pkt.session_token != session_token_) {
        std::cout << "[Server] Token non valido, connessione rifiutata.\n";
        enet_peer_disconnect(peer, 0);
        pending_auth_.erase(peer);
        return;
    }

    // Token ok (o non richiesto): promovi a giocatore
    pending_auth_.erase(peer);
    uint32_t pid = AssignPlayerId(peer);
    std::cout << "[Server] Giocatore " << pid << " autenticato e connesso.\n";

    Player new_player(pid);
    PlayerState spawn_state;
    spawn_state.player_id = pid;
    spawn_state.x = 64.0f;
    spawn_state.y = 64.0f;
    new_player.SetState(spawn_state);
    players_.emplace(pid, std::move(new_player));

    PktPlayerJoined joined_pkt;
    joined_pkt.header.type = PACKET_PLAYER_JOINED;
    joined_pkt.player_id   = pid;
    Broadcast(&joined_pkt, sizeof(joined_pkt));
}

void GameServer::OnClientDisconnected(ENetPeer* peer) {
    // Rimuovi da pending_auth_ se non aveva ancora autenticato
    pending_auth_.erase(peer);

    auto it = peer_to_id_.find(peer);
    if (it == peer_to_id_.end()) return;

    uint32_t pid = it->second;
    std::cout << "[Server] Giocatore " << pid << " disconnesso.\n";

    players_.erase(pid);
    pending_inputs_.erase(pid);
    peer_to_id_.erase(it);

    PktPlayerLeft left_pkt;
    left_pkt.header.type = PACKET_PLAYER_LEFT;
    left_pkt.player_id   = pid;
    Broadcast(&left_pkt, sizeof(left_pkt));
}

void GameServer::OnInputReceived(ENetPeer* peer, const PktInput& pkt) {
    auto it = peer_to_id_.find(peer);
    if (it == peer_to_id_.end()) return;

    // Salva l'input nel buffer — verrà processato al prossimo Tick()
    // NOTA: teniamo solo l'ultimo input ricevuto. In un sistema più robusto
    // si bufferizzerebbe una coda per gestire pacchetti fuori ordine.
    pending_inputs_[it->second] = pkt.frame;
}

// =============================================================================
// GAME LOOP
// =============================================================================

void GameServer::Tick() {
    // Per ogni giocatore, simula con l'ultimo input ricevuto
    // Se non c'è input, usa input vuoto (giocatore fermo)
    for (auto& [pid, player] : players_) {
        InputFrame input{};
        input.tick = server_tick_;

        auto it = pending_inputs_.find(pid);
        if (it != pending_inputs_.end()) {
            input = it->second;
        }

        // LA STESSA chiamata che fa il client per la prediction.
        // Questo è il punto dove server e client "parlano la stessa lingua".
        player.Simulate(input, world_);
    }

    // Svuota il buffer degli input processati
    pending_inputs_.clear();
}

void GameServer::BroadcastGameState() {
    PktGameState pkt;
    pkt.header.type             = PACKET_GAME_STATE;
    pkt.state.server_tick       = server_tick_;
    pkt.state.player_count      = 0;

    for (const auto& [pid, player] : players_) {
        if (pkt.state.player_count >= MAX_PLAYERS) break;
        pkt.state.players[pkt.state.player_count++] = player.GetState();
    }

    Broadcast(&pkt, sizeof(pkt), /*reliable=*/false);
    // NOTA: inviamo lo stato ogni tick come UNRELIABLE.
    // Perché? Perché è un dato "volatile": se un pacchetto viene perso,
    // il prossimo tick conterrà dati ancora più aggiornati.
    // Usare RELIABLE qui congestionerebbe il canale inutilmente.
}

// =============================================================================
// HELPERS
// =============================================================================

uint32_t GameServer::AssignPlayerId(ENetPeer* peer) {
    uint32_t pid = next_player_id_++;
    peer_to_id_[peer] = pid;

    PktPlayerAssigned assigned_pkt;
    assigned_pkt.header.type = PACKET_PLAYER_ASSIGNED;
    assigned_pkt.player_id   = pid;
    SendTo(peer, &assigned_pkt, sizeof(assigned_pkt));

    return pid;
}

void GameServer::SendTo(ENetPeer* peer, const void* data, size_t size, bool reliable) {
    enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* packet = enet_packet_create(data, size, flags);
    enet_peer_send(peer, 0, packet);
}

void GameServer::Broadcast(const void* data, size_t size, bool reliable) {
    enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* packet = enet_packet_create(data, size, flags);
    enet_host_broadcast(host_, 0, packet);
}
