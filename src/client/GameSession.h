#pragma once
// Manages one full play session from connect to disconnect.
// Responsibilities: 60 Hz fixed-step physics, network polling, client-side prediction
// and server reconciliation, visual effects, live leaderboard, results screen,
// pause menu, and renderer coordination.
// main.cpp is a thin orchestrator: ShowMainMenu → GameSession → repeat.
#include "World.h"
#include "Player.h"
#include "Protocol.h"
#include "VisualEffects.h"
#include "InputSampler.h"
#include "NetworkClient.h"
#include <raylib.h>
#include <unordered_map>
#include <string>
#include <cstdint>

class Renderer;

class GameSession {
public:
    struct Config {
        const char* map_path   = nullptr;
        const char* username   = "";
        bool        is_offline = false;  // true → connects to a LocalServer instance
    };

    explicit GameSession(const Config& cfg);

    // Execute one game-loop iteration (physics + network + render).
    // Returns true while the session is active.
    bool Tick(float dt, NetworkClient& net, Renderer& renderer);

    bool               IsOver()        const { return session_over_; }
    const std::string& GetEndMessage() const { return end_message_; }
    const std::string& GetEndSubMsg()  const { return end_sub_msg_; }
    Color              GetEndColor()   const { return end_color_; }

private:
    World       world_;
    Player      player_;
    uint32_t    local_player_id_ = 0;
    std::string username_;
    bool        is_offline_;
    uint8_t     current_level_ = 0;  // current level number from server (0 = lobby/unknown)

    static constexpr uint32_t IHIST = 128;   // input ring-buffer capacity for reconciliation
    InputFrame  input_history_[IHIST] = {};
    GameState   last_game_state_{};
    InputSampler input_sampler_;

    float    accumulator_ = 0.f;
    uint32_t sim_tick_    = 0;
    float    prev_x_      = 0.f;  // position at previous tick (trail / sub-frame interpolation)
    float    prev_y_      = 0.f;

    TrailState   trail_;
    std::unordered_map<uint32_t, TrailState>     remote_trails_;
    std::unordered_map<uint32_t, uint32_t>       remote_last_ticks_;
    DeathParticles                               local_death_{};
    std::unordered_map<uint32_t, DeathParticles> remote_deaths_;
    std::unordered_map<uint32_t, uint8_t>        remote_prev_kill_ticks_;
    std::unordered_map<uint32_t, Vector2>        remote_last_alive_pos_;

    std::unordered_map<uint32_t, uint32_t> live_best_ticks_;

    static constexpr double RESULTS_DURATION_S        = 15.0;
    static constexpr double GLOBAL_RESULTS_DURATION_S = 25.0;
    bool        in_results_screen_  = false;
    bool        local_ready_        = false;
    double      results_start_time_ = 0.0;
    uint8_t     results_count_      = 0;
    uint8_t     results_level_      = 0;
    ResultEntry results_entries_[MAX_PLAYERS] = {};

    // Global (session-end) leaderboard state
    bool              in_global_results_screen_  = false;
    bool              local_global_ready_         = false;
    double            global_results_start_time_  = 0.0;
    uint8_t           global_results_count_       = 0;
    uint8_t           global_results_total_levels_= 0;
    GlobalResultEntry global_results_entries_[MAX_PLAYERS] = {};

    bool     prev_finished_ = false;
    uint32_t best_ticks_    = 0;
    bool     show_record_   = false;

    float   last_safe_x_           = 0.f;
    float   last_safe_y_           = 0.f;
    uint8_t prev_kill_ticks_local_ = 0;

    PauseState pause_state_    = PauseState::PLAYING;
    int        pause_focused_  = 0;   // 0=Resume, 1=Quit to Menu
    int        confirm_focused_= 0;   // 0=No, 1=Yes

    bool        session_over_ = false;
    std::string end_message_;
    std::string end_sub_msg_;
    Color       end_color_{255, 80, 80, 255};

    // Queued disconnect reason received via PKT_VERSION_MISMATCH before the ENet DISCONNECT event.
    std::string pending_disc_reason_;
    std::string pending_disc_sub_;

    // Emote system — per-player bubble state
    std::unordered_map<uint32_t, EmoteBubble> emote_bubbles_;

    void HandlePauseInput(Renderer& renderer);
    void TickFixed(NetworkClient& net);
    void PollNetwork(NetworkClient& net);
    void HandlePacket(const uint8_t* data, size_t size, NetworkClient& net);
    void HandleDisconnect(uint32_t disconnect_data);
    void LoadLevel(const char* path);
    void LoadLevelFromGrid(int w, int h, const std::vector<std::string>& rows);
    void UpdateLiveBestTicks();
    void BuildLiveLeaderboard(LiveLeaderEntry* out, int& count) const;
    void DoRender(float draw_x, float draw_y, float dt,
                  NetworkClient& net, Renderer& renderer);
};
