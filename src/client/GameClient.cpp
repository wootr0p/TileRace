#include "GameClient.h"
#include "../common/Physics.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <cmath>

GameClient::GameClient()
    : local_player_(0) // ID 0 = non ancora assegnato
{}

GameClient::~GameClient() {
    audio_.Shutdown();
    renderer_.Shutdown();
    if (host_) {
        enet_host_destroy(host_);
        host_ = nullptr;
    }
}

bool GameClient::Init(const std::string& server_host, uint16_t port,
                      const std::string& map_path, uint32_t session_token) {
    session_token_ = session_token;
    // 1. Carica la mappa
    if (!world_.LoadFromFile(map_path)) {
        std::cerr << "[Client] ERRORE: impossibile caricare la mappa: " << map_path << "\n";
        return false;
    }

    // 2. Renderer
    if (!renderer_.Init(960, 540, "TileRace")) {
        std::cerr << "[Client] ERRORE: impossibile inizializzare la finestra.\n";
        return false;
    }

    // 3. Audio
    audio_.Init("assets");

    // 4. Rete
    host_ = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!host_) {
        std::cerr << "[Client] ERRORE: impossibile creare l'host ENet.\n";
        return false;
    }

    ENetAddress address;
    enet_address_set_host(&address, server_host.c_str());
    address.port = port;

    server_peer_ = enet_host_connect(host_, &address, 2, 0);
    if (!server_peer_) {
        std::cerr << "[Client] ERRORE: impossibile avviare la connessione.\n";
        return false;
    }

    // Aspetta la conferma di connessione (timeout 5 secondi)
    ENetEvent event;
    if (enet_host_service(host_, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        std::cout << "[Client] Connesso al server.\n";
        connected_ = true;

        // Invia la richiesta di connessione al gioco
        PktConnectRequest req;
        req.header.type       = PACKET_CONNECT_REQUEST;
        req.protocol_version  = 1;
        req.session_token     = session_token_;
        ENetPacket* pkt = enet_packet_create(&req, sizeof(req), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(server_peer_, 0, pkt);
    } else {
        std::cerr << "[Client] Timeout connessione.\n";
        return false;
    }

    return true;
}

// =============================================================================
// GAME LOOP
// =============================================================================

void GameClient::Run() {
    using Clock    = std::chrono::steady_clock;
    using Duration = std::chrono::duration<double>;

    const Duration tick_duration(Physics::FIXED_DT);
    auto last_time   = Clock::now();
    double accumulator = 0.0;

    while (!renderer_.ShouldClose()) {
        auto current_time = Clock::now();
        double frame_time = Duration(current_time - last_time).count();
        last_time = current_time;

        // Limita il frame_time per evitare la "spiral of death"
        // (es. se il programma si blocca brevemente, non facciamo 100 tick in un colpo)
        if (frame_time > 0.25) frame_time = 0.25;

        accumulator += frame_time;

        // Processa gli eventi di rete prima della simulazione
        ProcessNetworkEvents();

        // Simula tutti i tick fissi che sono "maturati" in questo frame
        while (accumulator >= Physics::FIXED_DT) {
            Tick();
            accumulator -= Physics::FIXED_DT;
        }

        // Alpha: quanto siamo "avanzati" verso il prossimo tick (0.0 → 1.0)
        const float alpha = static_cast<float>(accumulator / Physics::FIXED_DT);
        Render(alpha);

        // Poll() va chiamato DOPO Render() perché BeginDrawing() (dentro Render)
        // chiama PollInputEvents() di Raylib, che aggiorna IsKeyPressed.
        // Catturando qui, il flag jump_pending_ sopravvive fino al prossimo Tick().
        input_handler_.Poll();

        audio_.Update();
    }
}

// =============================================================================
// TICK (fisso a 60Hz)
// =============================================================================

void GameClient::Tick() {
    if (!connected_ || local_player_id_ == 0) {
        ++client_tick_;
        return;
    }

    // Salva stato attuale PRIMA di simulare (per interpolazione di tutti i player)
    prev_world_state_ = world_state_;
    prev_x_ = local_player_.GetState().x;
    prev_y_ = local_player_.GetState().y;

    // 1. Campiona l'input per questo tick
    InputFrame input = input_handler_.Sample(client_tick_);

    // 2. Applica la prediction locale
    PredictStep(input);

    // 3. Invia l'input al server
    SendInput(input);

    ++client_tick_;
}

// =============================================================================
// PREDICTION
// =============================================================================

void GameClient::PredictStep(const InputFrame& input) {
    // Simula il giocatore locale con l'input corrente
    // QUESTA È LA STESSA FUNZIONE CHE CHIAMA IL SERVER.
    // Il determinismo dipende dall'identità di questa chiamata.
    local_player_.Simulate(input, world_);

    // Salva nel buffer per una eventuale reconciliation futura
    PredictionEntry entry;
    entry.tick        = client_tick_;
    entry.input       = input;
    entry.state_after = local_player_.GetState();

    prediction_buffer_.push_back(entry);

    // Evita che il buffer cresca all'infinito
    if (prediction_buffer_.size() > MAX_BUFFER_SIZE) {
        prediction_buffer_.pop_front();
    }

    // Aggiorna il world_state_ con il nostro stato predetto
    // (così il Renderer mostra sempre la nostra posizione aggiornata)
    PlayerState* local_in_world = world_state_.FindPlayer(local_player_id_);
    if (local_in_world) {
        *local_in_world = local_player_.GetState();
    } else if (world_state_.player_count < MAX_PLAYERS) {
        world_state_.players[world_state_.player_count++] = local_player_.GetState();
    }
}

// =============================================================================
// RECONCILIATION — Il momento della verità
// =============================================================================

void GameClient::Reconcile(const GameState& server_state) {
    // 1. Trova il nostro PlayerState nell'aggiornamento del server
    const PlayerState* server_ps = server_state.FindPlayer(local_player_id_);
    if (!server_ps) return; // Il server non ci conosce ancora

    const uint32_t last_processed = server_ps->last_processed_tick;

    // 2. Rimuovi dal buffer tutti gli input già "confermati" dal server
    //    (quelli con tick <= last_processed_tick)
    while (!prediction_buffer_.empty() &&
           prediction_buffer_.front().tick <= last_processed) {
        prediction_buffer_.pop_front();
    }

    // 3. Confronta: il nostro stato predetto al tick (last_processed) combacia
    //    con quello del server?
    //
    //    NOTA: qui usiamo una soglia di tolleranza per il floating point.
    //    Un confronto esatto con == su float è fonte di bug sottili.
    const float dx = local_player_.GetState().x - server_ps->x;
    const float dy = local_player_.GetState().y - server_ps->y;
    const float error_sq = dx * dx + dy * dy;
    const float THRESHOLD_SQ = 1.0f; // 1 unità di distanza = errore accettabile

    if (error_sq <= THRESHOLD_SQ) {
        // Tutto ok — la prediction era corretta (o abbastanza vicina)
        // Aggiorna comunque lo stato dei ALTRI giocatori
        world_state_ = server_state;
        // Ripristina il nostro stato predetto (non vogliamo il rollback)
        PlayerState* our_slot = world_state_.FindPlayer(local_player_id_);
        if (our_slot) *our_slot = local_player_.GetState();
        return;
    }

    // MISMATCH! Dobbiamo fare il ROLLBACK e re-simulare
    // =========================================================
    std::cout << "[Client] Reconciliation attivata al tick " << last_processed
              << " (errore: " << std::sqrt(error_sq) << " unità)\n";

    // 4. Imposta lo stato del giocatore locale = stato autoritativo del server
    local_player_.SetState(*server_ps);

    // 5. Re-simula TUTTI gli input non ancora confermati, in ordine
    //    Questo "riporta" il giocatore alla posizione dove dovrebbe essere NOW
    for (const auto& entry : prediction_buffer_) {
        local_player_.Simulate(entry.input, world_);
    }

    // 6. Aggiorna il buffer con i nuovi stati re-simulati
    //    (Opzionale ma utile per riconciliazioni successive)
    Player temp_player(local_player_id_);
    temp_player.SetState(*server_ps);
    for (auto& entry : prediction_buffer_) {
        temp_player.Simulate(entry.input, world_);
        entry.state_after = temp_player.GetState();
    }

    // 7. Aggiorna world_state_ con i dati del server + la nostra posizione corretta
    world_state_ = server_state;
    PlayerState* our_slot = world_state_.FindPlayer(local_player_id_);
    if (our_slot) *our_slot = local_player_.GetState();
}

// =============================================================================
// RENDERING (variabile, massimo FPS)
// =============================================================================

void GameClient::Render(float alpha) {
    // Interpola TUTTI i player tra il tick precedente e quello corrente.
    // Questo elimina il jitter visivo a FPS > 60 per ogni entità in scena.
    GameState interp = world_state_;
    for (int i = 0; i < interp.player_count; ++i) {
        PlayerState& ps = interp.players[i];
        if (ps.player_id == local_player_id_) {
            // Player locale: usiamo prev_x_/prev_y_ aggiornati ogni tick
            ps.x = prev_x_ + (ps.x - prev_x_) * alpha;
            ps.y = prev_y_ + (ps.y - prev_y_) * alpha;
        } else {
            // Giocatori remoti: lerp tra l'ultimo snapshot server ricevuto
            const PlayerState* prev_ps = prev_world_state_.FindPlayer(ps.player_id);
            if (prev_ps) {
                ps.x = prev_ps->x + (ps.x - prev_ps->x) * alpha;
                ps.y = prev_ps->y + (ps.y - prev_ps->y) * alpha;
            }
        }
    }

    float cam_x, cam_y;
    if (local_player_id_ != 0) {
        cam_x = prev_x_ + (local_player_.GetState().x - prev_x_) * alpha + 16.0f;
        cam_y = prev_y_ + (local_player_.GetState().y - prev_y_) * alpha + 16.0f;
    } else {
        cam_x = 0.0f;
        cam_y = 0.0f;
    }
    renderer_.Draw(interp, world_, local_player_id_, cam_x, cam_y);
}

// =============================================================================
// RETE
// =============================================================================

void GameClient::ProcessNetworkEvents() {
    if (!host_) return;

    ENetEvent event;
    while (enet_host_service(host_, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                OnConnected(event.peer);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                OnDisconnected();
                break;

            case ENET_EVENT_TYPE_RECEIVE: {
                if (event.packet->dataLength < sizeof(PacketHeader)) {
                    enet_packet_destroy(event.packet);
                    break;
                }

                PacketHeader header;
                std::memcpy(&header, event.packet->data, sizeof(header));

                switch (static_cast<PacketType>(header.type)) {
                    case PACKET_PLAYER_ASSIGNED:
                        if (event.packet->dataLength >= sizeof(PktPlayerAssigned)) {
                            PktPlayerAssigned pkt;
                            std::memcpy(&pkt, event.packet->data, sizeof(pkt));
                            OnPlayerAssigned(pkt);
                        }
                        break;

                    case PACKET_GAME_STATE:
                        if (event.packet->dataLength >= sizeof(PktGameState)) {
                            PktGameState pkt;
                            std::memcpy(&pkt, event.packet->data, sizeof(pkt));
                            OnGameStateReceived(pkt);
                        }
                        break;

                    case PACKET_AUDIO_EVENT:
                        if (event.packet->dataLength >= sizeof(PktAudioEvent)) {
                            PktAudioEvent pkt;
                            std::memcpy(&pkt, event.packet->data, sizeof(pkt));
                            OnAudioEventReceived(pkt);
                        }
                        break;

                    default:
                        break;
                }

                enet_packet_destroy(event.packet);
                break;
            }
            default: break;
        }
    }
}

void GameClient::OnConnected(ENetPeer* /*peer*/) {
    std::cout << "[Client] Evento CONNECT ricevuto.\n";
    connected_ = true;
}

void GameClient::OnDisconnected() {
    std::cout << "[Client] Disconnesso dal server.\n";
    connected_       = false;
    local_player_id_ = 0;
}

void GameClient::OnPlayerAssigned(const PktPlayerAssigned& pkt) {
    local_player_id_ = pkt.player_id;
    std::cout << "[Client] Assegnato come giocatore " << local_player_id_ << "\n";

    // Inizializza il local_player con il nuovo ID
    local_player_ = Player(local_player_id_);
}

void GameClient::OnGameStateReceived(const PktGameState& pkt) {
    // La reconciliation è il momento in cui il server "corregge" la nostra prediction
    Reconcile(pkt.state);
}

void GameClient::OnAudioEventReceived(const PktAudioEvent& pkt) {
    audio_.PlayEvent(pkt);
}

void GameClient::SendInput(const InputFrame& frame) {
    if (!connected_ || !server_peer_) return;

    PktInput pkt;
    pkt.header.type = PACKET_INPUT;
    pkt.frame       = frame;

    // Input inviato come UNRELIABLE per minimizzare la latenza.
    // In caso di perdita, il server usa l'ultimo input valido.
    // Un sistema più avanzato invierebbe gli ultimi N input in ogni pacchetto
    // per la ridondanza (input redundancy).
    ENetPacket* packet = enet_packet_create(&pkt, sizeof(pkt), 0);
    enet_peer_send(server_peer_, 0, packet);
}
