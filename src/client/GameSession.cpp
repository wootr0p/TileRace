// GameSession.cpp — implementazione del game loop di una singola sessione.
//
// Contiene la logica estratta dal grande main() originale:
//   physics tick, reconciliation, effetti visivi, rete, pausa, render.

#include "GameSession.h"
#include "Renderer.h"
#include "Physics.h"    // FIXED_DT, TILE_SIZE
#include "Colors.h"
#include "Protocol.h"   // LOBBY_MAP_PATH, PKT_* constants
#include "GameMode.h"
#include "SpawnFinder.h" // FindCenterSpawn (shared con server)
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// Catmull-Rom helpers for spline tessellation caching
// ---------------------------------------------------------------------------
static Vector2 CatmullRomPoint(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    return {
        0.5f * (2.f*p1.x + (-p0.x + p2.x)*t + (2.f*p0.x - 5.f*p1.x + 4.f*p2.x - p3.x)*t2 + (-p0.x + 3.f*p1.x - 3.f*p2.x + p3.x)*t3),
        0.5f * (2.f*p1.y + (-p0.y + p2.y)*t + (2.f*p0.y - 5.f*p1.y + 4.f*p2.y - p3.y)*t2 + (-p0.y + 3.f*p1.y - 3.f*p2.y + p3.y)*t3)
    };
}
#include <cstring>
#include <raylib.h>

// ---------------------------------------------------------------------------
// Costruttore
// ---------------------------------------------------------------------------
GameSession::GameSession(const Config& cfg)
    : username_(cfg.username ? cfg.username : "")
    , is_offline_(cfg.is_offline)
    , save_(cfg.save)
{
    // Pre-assegna il gamepad reclamato allo splash screen (se presente).
    // Una volta impostato, gp_index_ non cambia mai: se il gamepad si disconnette
    // e si riconnette allo stesso indice, il gioco riprende a rispondergli
    // automaticamente; nessun altro gamepad può "rubare" il controllo.
    input_sampler_.SetGamepadIndex(cfg.gamepad_index);
    // Applica il mute iniziale dai dati salvati.
    if (save_)
        sfx_.SetMuted(save_->sfx_muted);

    if (cfg.map_path) {
        world_.LoadFromFile(cfg.map_path);
        const SpawnPos sp = FindCenterSpawn(world_);
        PlayerState ps{};
        std::strncpy(ps.name, username_.c_str(), sizeof(ps.name) - 1);
        ps.x = sp.x;
        ps.y = sp.y;
        player_.SetState(ps);
        prev_x_ = ps.x;
        prev_y_ = ps.y;
        last_safe_x_ = ps.x;
        last_safe_y_ = ps.y;
    }
}

// ---------------------------------------------------------------------------
// Tick — un'iterazione del game loop. Ritorna false quando la sessione finisce.
// ---------------------------------------------------------------------------
bool GameSession::Tick(float dt, NetworkClient& net, Renderer& renderer) {
    accumulator_ += dt;

    // 1. Cattura input del frame corrente
    input_sampler_.Poll();

    // 2. Gestione menu di pausa (usa nav + toggle del sampler + GetMousePosition)
    HandlePauseInput(renderer, net);
    if (session_over_) return false;

    // 3. Quando in pausa: scarta i flag sticky di gameplay
    //    (evita salti/dash involontari alla ripresa)
    if (pause_state_ != PauseState::PLAYING) {
        input_sampler_.ConsumeJumpPressed();
        input_sampler_.ConsumeDashPending();
        input_sampler_.ConsumeRestartRequest();
        input_sampler_.ConsumeRestartSpawn();
    }

    // 4. Results screen: SPAZIO → ready up (consuma il jump_pressed)
    if (in_results_screen_ && !local_ready_) {
        if (input_sampler_.ConsumeJumpPressed()) {
            PktReady rp{};
            net.SendReliable(&rp, sizeof(rp));
            local_ready_ = true;
            printf("[session] READY inviato\n");
        }
    }

    // 4b. Global results screen: SPAZIO → ready up
    if (in_global_results_screen_ && !local_global_ready_) {
        if (input_sampler_.ConsumeJumpPressed()) {
            PktReady rp{};
            net.SendReliable(&rp, sizeof(rp));
            local_global_ready_ = true;
            printf("[session] GLOBAL READY inviato\n");
        }
    }

    // 5a. Restart al checkpoint (Backspace / Triangle) — solo in gioco, player vivo, non lobby
    if (pause_state_ == PauseState::PLAYING && !last_game_state_.is_lobby
        && input_sampler_.ConsumeRestartRequest()) {
        const PlayerState& cur = player_.GetState();
        const GameMode cur_mode = static_cast<GameMode>(last_game_state_.game_mode);
        // In versus mode, a finished player cannot restart.
        const bool can_restart = (cur.kill_respawn_ticks == 0 && cur.respawn_grace_ticks == 0)
            && !(cur_mode == GameMode::VERSUS && cur.finished);
        if (can_restart) {
            PktRestart rpkt{};
            net.SendReliable(&rpkt, sizeof(rpkt));
            show_record_ = false;
        }
    }

    // 5b. Restart allo spawn (Delete) — solo in gioco, player vivo, non lobby
    if (pause_state_ == PauseState::PLAYING && !last_game_state_.is_lobby
        && input_sampler_.ConsumeRestartSpawn()) {
        const PlayerState& cur = player_.GetState();
        const GameMode cur_mode = static_cast<GameMode>(last_game_state_.game_mode);
        // In versus mode, a finished player cannot restart.
        const bool can_restart = (cur.kill_respawn_ticks == 0 && cur.respawn_grace_ticks == 0)
            && !(cur_mode == GameMode::VERSUS && cur.finished);
        if (can_restart) {
            PktRestartSpawn rpkt{};
            net.SendReliable(&rpkt, sizeof(rpkt));
            show_record_ = false;
        }
    }

    // 5c. Emote — send if pending
    if (pause_state_ == PauseState::PLAYING) {
        const int emote = input_sampler_.ConsumeEmotePending();
        if (emote >= 0 && emote < EMOTE_COUNT) {
            PktEmote epkt{};
            epkt.emote_id = static_cast<uint8_t>(emote);
            net.SendReliable(&epkt, sizeof(epkt));
            // Show locally immediately
            EmoteBubble& eb = emote_bubbles_[local_player_id_];
            eb.emote_id = static_cast<uint8_t>(emote);
            eb.timer    = EMOTE_DURATION;
            eb.active   = true;
            eb.last_x   = player_.GetState().x;
            eb.last_y   = player_.GetState().y;
        }
    }

    // Lobby controls — mode change and game start now handled via pause menu
    //     (game starts automatically when all players reach the exit)

    // 6. Fixed-step loop (azzerato se in pausa, risultati o classifica globale)
    if (pause_state_ != PauseState::PLAYING || in_results_screen_ || in_global_results_screen_) accumulator_ = 0.f;
    while (accumulator_ >= FIXED_DT) {
        TickFixed(net);
        accumulator_ -= FIXED_DT;
    }

    // 7. Ricezione pacchetti dal server
    PollNetwork(net);
    if (session_over_) return false;

    // Advance generating overlay timer (counts up while waiting for level data)
    if (generating_level_) generating_elapsed_ += dt;

    // 8. Rilevamento completamento livello e aggiornamento record
    const PlayerState& local = player_.GetState();
    if (local.finished && !prev_finished_) {
        if (best_ticks_ == 0 || local.level_ticks < best_ticks_) {
            best_ticks_  = local.level_ticks;
            show_record_ = true;
        }
        sfx_.PlayLevelEnd();
    }
    prev_finished_ = local.finished;

    // 9. Posizione interpolata per il rendering
    const float alpha  = accumulator_ / FIXED_DT;
    const float draw_x = prev_x_ + (local.x - prev_x_) * alpha;
    const float draw_y = prev_y_ + (local.y - prev_y_) * alpha;

    // 10. Rilevamento morte + spawn particelle + aggiornamento camera
    const bool just_died      = (local.kill_respawn_ticks > 0 && prev_kill_ticks_local_ == 0);
    const bool just_respawned = (prev_kill_ticks_local_ > 0  && local.kill_respawn_ticks == 0);

    // Aggiorna ultima posizione sicura (solo quando vivo e non in grace)
    if (local.kill_respawn_ticks == 0 && local.respawn_grace_ticks == 0) {
        last_safe_x_ = draw_x;
        last_safe_y_ = draw_y;
    }
    if (just_died) {
        local_death_.Spawn(last_safe_x_ + TILE_SIZE * 0.5f,
                           last_safe_y_ + TILE_SIZE * 0.5f,
                           CLRS_PLAYER_LOCAL);
        renderer.TriggerShake(0.4f);
        sfx_.PlayDeath();
    }
    prev_kill_ticks_local_ = local.kill_respawn_ticks;

    // Ready / Go — local player only (grace period transitions).
    {
        const uint8_t cur_grace = local.respawn_grace_ticks;
        if (prev_grace_local_ == 0 && cur_grace > 0)       sfx_.PlayReady();
        else if (prev_grace_local_ > 0 && cur_grace == 0)  sfx_.PlayGo();
        prev_grace_local_ = cur_grace;
    }

    // Checkpoint locale — rilevato qui dopo reconciliation (checkpoint_x/y è server-side only).
    {
        const float cx = local.checkpoint_x;
        const float cy = local.checkpoint_y;
        if (cx != prev_checkpoint_x_ || cy != prev_checkpoint_y_) {
            // Non suonare al primo frame (init) e non suonare se il checkpoint viene rimosso (reset a 0,0).
            if (cx != 0.f || cy != 0.f)
                sfx_.PlayCheckpoint();
            prev_checkpoint_x_ = cx;
            prev_checkpoint_y_ = cy;
        }
    }

    // Morte player remoti
    const PlayerState& local_ps    = player_.GetState();
    const float        listener_cx = local_ps.x + TILE_SIZE * 0.5f;
    const float        listener_cy = local_ps.y + TILE_SIZE * 0.5f;
    for (uint32_t i = 0; i < last_game_state_.count; i++) {
        const PlayerState& rp = last_game_state_.players[i];
        if (rp.player_id == 0 || rp.player_id == local_player_id_) continue;
        if (rp.kill_respawn_ticks == 0 && rp.respawn_grace_ticks == 0)
            remote_last_alive_pos_[rp.player_id] = {rp.x, rp.y};
        uint8_t& prev_kt = remote_prev_kill_ticks_[rp.player_id];
        if (rp.kill_respawn_ticks > 0 && prev_kt == 0) {
            const Vector2 alive_pos = remote_last_alive_pos_.count(rp.player_id)
                ? remote_last_alive_pos_[rp.player_id]
                : Vector2{rp.x, rp.y};
            remote_deaths_[rp.player_id].Spawn(alive_pos.x + TILE_SIZE * 0.5f,
                                               alive_pos.y + TILE_SIZE * 0.5f,
                                               CLRS_PLAYER_REMOTE);
            sfx_.PlayDeathAt(alive_pos.x + TILE_SIZE * 0.5f,
                             alive_pos.y + TILE_SIZE * 0.5f,
                             listener_cx, listener_cy);
        }
        prev_kt = rp.kill_respawn_ticks;
    }
    local_death_.Update(dt);
    for (auto& [id, dp] : remote_deaths_) dp.Update(dt);

    // 11. Camera
    const float cx = draw_x + TILE_SIZE * 0.5f;
    const float cy = draw_y + TILE_SIZE * 0.5f;
    if (local.kill_respawn_ticks > 0) {
        // Morto: telecamera immobile
    } else if (just_respawned) {
        renderer.SnapCamera(cx, cy);
    } else {
        renderer.UpdateCamera(cx, cy, dt);
    }
    renderer.TickShake(dt);

    // 12. Aggiorna classifica live
    UpdateLiveBestTicks();

    // 13. Render
    DoRender(draw_x, draw_y, dt, net, renderer);
    return true;
}

// ---------------------------------------------------------------------------
// HandlePauseInput
// ---------------------------------------------------------------------------
void GameSession::HandlePauseInput(Renderer& renderer, NetworkClient& net) {
    const bool toggle  = input_sampler_.ConsumePauseToggle();
    const bool nav_up  = input_sampler_.PauseNavUp();
    const bool nav_dn  = input_sampler_.PauseNavDown();
    const bool ok      = input_sampler_.PauseNavOk();

    const Vector2 mouse   = GetMousePosition();
    const bool    clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // Determine whether "Lobby Settings" should appear in the pause menu
    const bool show_lobby_settings = last_game_state_.is_lobby
        && (local_player_id_ == last_game_state_.leader_id);

    // Dynamic item count: Resume, [Lobby Settings], SFX, Quit
    const int  item_count = show_lobby_settings ? 4 : 3;
    auto item_rect = [&](int i) -> Rectangle { return renderer.GetPauseItemRect(i, item_count); };

    if (pause_state_ == PauseState::PLAYING && toggle && !in_results_screen_) {
        pause_state_   = PauseState::PAUSED;
        pause_focused_ = 0;

    } else if (pause_state_ == PauseState::PAUSED) {
        for (int i = 0; i < item_count; i++)
            if (CheckCollisionPointRec(mouse, item_rect(i))) pause_focused_ = i;

        if      (toggle)       pause_state_   = PauseState::PLAYING;
        else if (nav_up)       pause_focused_ = (pause_focused_ + item_count - 1) % item_count;
        else if (nav_dn)       pause_focused_ = (pause_focused_ + 1) % item_count;
        else if (ok || (clicked && CheckCollisionPointRec(mouse, item_rect(pause_focused_)))) {
            if (pause_focused_ == 0) {
                // Resume
                pause_state_ = PauseState::PLAYING;
            } else if (show_lobby_settings && pause_focused_ == 1) {
                // Lobby Settings sub-menu
                pause_state_ = PauseState::LOBBY_SETTINGS;
                pause_focused_ = 0;
            } else if (pause_focused_ == (show_lobby_settings ? 2 : 1)) {
                // SFX toggle
                sfx_.SetMuted(!sfx_.IsMuted());
                if (save_) { save_->sfx_muted = sfx_.IsMuted(); SaveSaveData(*save_); }
            } else {
                // Quit to menu
                pause_state_ = PauseState::CONFIRM_QUIT; confirm_focused_ = 0;
            }
        }

    } else if (pause_state_ == PauseState::LOBBY_SETTINGS) {
        auto ls_rect = [&](int i) -> Rectangle { return renderer.GetPauseItemRect(i, 3); };
        for (int i = 0; i < 3; i++)
            if (CheckCollisionPointRec(mouse, ls_rect(i))) pause_focused_ = i;

        const bool gp = IsGamepadAvailable(0);
        const float rsx = gp ? GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X) : 0.f;
        const bool stick_right = (prev_pause_right_stick_x_ < 0.5f && rsx >= 0.5f);
        const bool stick_left  = (prev_pause_right_stick_x_ > -0.5f && rsx <= -0.5f);
        prev_pause_right_stick_x_ = rsx;

        const bool nav_left  = IsKeyPressed(KEY_LEFT)
            || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT))
            || stick_left;
        const bool nav_right = IsKeyPressed(KEY_RIGHT)
            || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))
            || stick_right;
        const float wheel = GetMouseWheelMove();
        const bool wheel_left = wheel < -0.01f;
        const bool wheel_right = wheel > 0.01f;

        auto current_max_levels = [&]() -> uint8_t {
            uint8_t v = last_game_state_.max_generated_levels;
            if (v < static_cast<uint8_t>(MIN_GENERATED_LEVELS) ||
                v > static_cast<uint8_t>(MAX_GENERATED_LEVELS_LIMIT)) {
                v = static_cast<uint8_t>(MAX_GENERATED_LEVELS);
            }
            return v;
        };
        auto send_max_levels = [&](int delta) {
            int value = static_cast<int>(current_max_levels()) + delta;
            if (value < MIN_GENERATED_LEVELS) value = MIN_GENERATED_LEVELS;
            if (value > MAX_GENERATED_LEVELS_LIMIT) value = MAX_GENERATED_LEVELS_LIMIT;
            if (value == static_cast<int>(current_max_levels())) return;
            PktSetMaxLevels lpkt{};
            lpkt.max_levels = static_cast<uint8_t>(value);
            net.SendReliable(&lpkt, sizeof(lpkt));
        };

        if      (toggle)       { pause_state_ = PauseState::PAUSED; pause_focused_ = 0; }
        else if (nav_up)       pause_focused_ = (pause_focused_ + 2) % 3;
        else if (nav_dn)       pause_focused_ = (pause_focused_ + 1) % 3;
        else if (pause_focused_ == 1 && (nav_left || wheel_left))  send_max_levels(-1);
        else if (pause_focused_ == 1 && (nav_right || wheel_right)) send_max_levels(+1);
        else if (ok || (clicked && CheckCollisionPointRec(mouse, ls_rect(pause_focused_)))) {
            if (pause_focused_ == 0) {
                // Cycle mode: VERSUS → RACE → COOP → VERSUS
                PktSetGameMode mpkt{};
                const GameMode cur = static_cast<GameMode>(last_game_state_.game_mode);
                GameMode next;
                if      (cur == GameMode::VERSUS) next = GameMode::RACE;
                else if (cur == GameMode::RACE)   next = GameMode::COOP;
                else                              next = GameMode::VERSUS;
                mpkt.game_mode = static_cast<uint8_t>(next);
                net.SendReliable(&mpkt, sizeof(mpkt));
            } else if (pause_focused_ == 1) {
                // Quick increment when confirming the levels row.
                send_max_levels(+1);
            } else {
                // Back
                pause_state_ = PauseState::PAUSED; pause_focused_ = 0;
            }
        }

    } else if (pause_state_ == PauseState::CONFIRM_QUIT) {
        auto confirm_rect = [&](int i) -> Rectangle { return renderer.GetPauseItemRect(i, 2); };
        for (int i = 0; i < 2; i++)
            if (CheckCollisionPointRec(mouse, confirm_rect(i))) confirm_focused_ = i;

        if      (toggle)                  pause_state_    = PauseState::PAUSED;
        else if (nav_up  || nav_dn)       confirm_focused_ = 1 - confirm_focused_;
        else if (ok || (clicked && CheckCollisionPointRec(mouse, confirm_rect(confirm_focused_)))) {
            if (confirm_focused_ == 1) session_over_ = true;
            else                       pause_state_  = PauseState::PAUSED;
        }
    }
}

// ---------------------------------------------------------------------------
// TickFixed — un tick fisso 60 Hz
// ---------------------------------------------------------------------------
void GameSession::TickFixed(NetworkClient& net) {
    // Trail
    if (player_.GetState().dash_active_ticks > 0)
        trail_.Push(player_.GetState().x, player_.GetState().y);
    else
        trail_.Clear();

    // Build InputFrame
    const float  move_x  = input_sampler_.GetMoveX();
    float        dash_dx = 0.f, dash_dy = 0.f;
    input_sampler_.GetDashDir(dash_dx, dash_dy);

    InputFrame frame;
    frame.tick    = sim_tick_++;
    frame.move_x  = move_x;
    frame.dash_dx = dash_dx;
    frame.dash_dy = dash_dy;

    if (move_x < -0.01f) frame.buttons |= BTN_LEFT;
    if (move_x >  0.01f) frame.buttons |= BTN_RIGHT;
    if (input_sampler_.IsJumpHeld())          frame.buttons |= BTN_JUMP;
    if (input_sampler_.ConsumeJumpPressed()) { frame.buttons |= BTN_JUMP_PRESS; }
    if (input_sampler_.ConsumeDashPending()) { frame.buttons |= BTN_DASH; }
    if (input_sampler_.IsDrawHeld())           frame.buttons |= BTN_DRAW;
    if (input_sampler_.IsSprintHeld())          frame.buttons |= BTN_SPRINT;
    if (input_sampler_.IsMagnetHeld())          frame.buttons |= BTN_MAGNET;

    // Blocca input durante morte/grace
    {
        const PlayerState& cur = player_.GetState();
        if (cur.kill_respawn_ticks > 0 || cur.respawn_grace_ticks > 0) {
            frame.buttons = 0;
            frame.move_x  = 0.f;
            frame.dash_dx = 0.f;
            frame.dash_dy = 0.f;
        }
    }

    // Blocca input se il giocatore ha raggiunto il traguardo (valido in tutte le modalità).
    {
        const PlayerState& cur = player_.GetState();
        if (cur.finished) {
            frame.buttons = 0;
            frame.move_x  = 0.f;
            frame.dash_dx = 0.f;
            frame.dash_dy = 0.f;
        }
    }

    // Invia al server
    PktInput pkt{};
    pkt.frame = frame;
    net.SendReliable(&pkt, sizeof(pkt));

    // Archivia per reconciliation
    input_history_[frame.tick % IHIST] = frame;

    // Salva lo stato prima della simulazione per rilevare eventi sonori.
    const float   pre_vy   = player_.GetState().vel_y;
    const uint8_t pre_dash = player_.GetState().dash_active_ticks;
    const int8_t  pre_wjd  = player_.GetState().last_wall_jump_dir;

    // Simulazione locale (prediction)
    prev_x_ = player_.GetState().x;
    prev_y_ = player_.GetState().y;
    player_.Simulate(frame, world_);

    // SFX giocatore locale.
    const int8_t post_wjd   = player_.GetState().last_wall_jump_dir;
    const bool   is_wall_jump = (post_wjd != 0 && post_wjd != pre_wjd);
    // Wall jump: last_wall_jump_dir transisce a ±1 (ha priorità sul suono di salto normale).
    if (is_wall_jump)
        sfx_.PlayWallJump();
    // Jump normale: vel_y scende sotto la soglia di impulso (solo se non è un wall jump).
    else if (pre_vy > -500.f && player_.GetState().vel_y <= -500.f)
        sfx_.PlayJump();
    // Dash: dash_active_ticks transisce da 0 a >0.
    if (pre_dash == 0 && player_.GetState().dash_active_ticks > 0)
        sfx_.PlayDash();
    // Nota: checkpoint rilevato nel main Tick() dopo la reconciliation (server-side only).

    // --- Drawing trail (local player) ---
    {
        const PlayerState& ls = player_.GetState();
        auto& strokes = draw_trails_[local_player_id_];
        bool& prev_dr = draw_prev_drawing_[local_player_id_];
        if (ls.drawing && ls.kill_respawn_ticks == 0 && ls.respawn_grace_ticks == 0) {
            // Count total points for this player across all strokes.
            int total = 0;
            for (const auto& st : strokes) total += static_cast<int>(st.pts.size());
            if (total < DRAW_MAX_POINTS) {
                if (!prev_dr || strokes.empty()) {
                    // Start a new stroke.
                    strokes.push_back({});
                    strokes.back().created_time_s = GetTime();
                    strokes.back().pts.push_back({ls.x + TILE_SIZE * 0.5f,
                                                  ls.y + TILE_SIZE * 0.5f});
                } else {
                    auto& cur = strokes.back().pts;
                    const Vector2 last = cur.back();
                    const float cx = ls.x + TILE_SIZE * 0.5f;
                    const float cy = ls.y + TILE_SIZE * 0.5f;
                    const float ddx = cx - last.x;
                    const float ddy = cy - last.y;
                    if (ddx * ddx + ddy * ddy >= DRAW_MIN_DIST * DRAW_MIN_DIST)
                        cur.push_back({cx, cy});
                }
            }
        }
        prev_dr = ls.drawing;
    }
}

// ---------------------------------------------------------------------------
// PollNetwork — receive e dispatch di tutti gli eventi pendenti
// ---------------------------------------------------------------------------
void GameSession::PollNetwork(NetworkClient& net) {
    NetEvent ev;
    while ((ev = net.Poll()).type != NetEventType::None) {
        if (ev.type == NetEventType::Packet && !ev.data.empty()) {
            HandlePacket(ev.data.data(), ev.data.size(), net);
        } else if (ev.type == NetEventType::Disconnected) {
            HandleDisconnect(ev.disconnect_data);
        }
        if (session_over_) break;
    }
}

// ---------------------------------------------------------------------------
// HandlePacket
// ---------------------------------------------------------------------------
void GameSession::HandlePacket(const uint8_t* data, size_t size, NetworkClient& net) {
    if (size < 1) return;
    const uint8_t pkt_type = data[0];

    // PKT_WELCOME: assegna player_id, invia nome+versione
    if (pkt_type == PKT_WELCOME && size >= sizeof(PktWelcome)) {
        PktWelcome welcome{};
        std::memcpy(&welcome, data, sizeof(PktWelcome));
        local_player_id_ = welcome.player_id;
        printf("[session] player_id=%u  session_token=%u\n",
               welcome.player_id, welcome.session_token);

        PlayerState s = player_.GetState();
        s.player_id   = local_player_id_;
        player_.SetState(s);

        PktPlayerInfo info{};
        info.protocol_version = PROTOCOL_VERSION;
        std::strncpy(info.name, username_.c_str(), sizeof(info.name) - 1);
        net.SendReliable(&info, sizeof(info));
        printf("[session] nome='%s'  protocol=%u\n", username_.c_str(), PROTOCOL_VERSION);
        return;
    }

    // PKT_VERSION_MISMATCH: salva il messaggio; il DISCONNECT arriverà subito dopo
    if (pkt_type == PKT_VERSION_MISMATCH && size >= sizeof(PktVersionMismatch)) {
        PktVersionMismatch vm{};
        std::memcpy(&vm, data, sizeof(vm));
        printf("[session] VERSION_MISMATCH: server=%u client=%u\n",
               vm.server_version, PROTOCOL_VERSION);
        char sub[128];
        snprintf(sub, sizeof(sub),
                 "Server protocol: %u  \xe2\x80\x94  Your client: %u",
                 vm.server_version, PROTOCOL_VERSION);
        pending_disc_reason_ = "Version Mismatch: please update your client.";
        pending_disc_sub_    = sub;
        return;
    }

    // PKT_GAME_STATE: reconciliation + aggiornamento remoti
    if (pkt_type == PKT_GAME_STATE && size >= sizeof(PktGameState)) {
        PktGameState resp{};
        std::memcpy(&resp, data, sizeof(PktGameState));

        // Grab SFX: detect authoritative grabbed-state transitions per player.
        // This guarantees all clients hear the same grab_on / grab_off events.
        std::unordered_set<uint32_t> seen_players;
        seen_players.reserve(resp.state.count);
        float listener_x = player_.GetState().x + TILE_SIZE * 0.5f;
        float listener_y = player_.GetState().y + TILE_SIZE * 0.5f;
        for (uint32_t i = 0; i < resp.state.count; ++i) {
            const PlayerState& ps = resp.state.players[i];
            if (ps.player_id == local_player_id_) {
                listener_x = ps.x + TILE_SIZE * 0.5f;
                listener_y = ps.y + TILE_SIZE * 0.5f;
                break;
            }
        }
        for (uint32_t i = 0; i < resp.state.count; i++) {
            const PlayerState& ps = resp.state.players[i];
            if (ps.player_id == 0) continue;
            seen_players.insert(ps.player_id);
            auto it = prev_grabbed_state_.find(ps.player_id);
            if (it == prev_grabbed_state_.end()) {
                prev_grabbed_state_[ps.player_id] = ps.grabbed;
                continue;  // first sighting: initialize without playing SFX
            }
            const float src_x = ps.x + TILE_SIZE * 0.5f;
            const float src_y = ps.y + TILE_SIZE * 0.5f;
            if (!it->second && ps.grabbed) sfx_.PlayGrabOnAt(src_x, src_y, listener_x, listener_y);
            else if (it->second && !ps.grabbed) sfx_.PlayGrabOffAt(src_x, src_y, listener_x, listener_y);
            it->second = ps.grabbed;
        }
        for (auto it = prev_grabbed_state_.begin(); it != prev_grabbed_state_.end(); ) {
            if (!seen_players.count(it->first)) it = prev_grabbed_state_.erase(it);
            else ++it;
        }

        last_game_state_ = resp.state;

        // Aggiorna trail remoti e rileva eventi SFX — SOLO su nuovo tick autoritativo.
        // Gestire qui (non nel loop di Tick) evita falsi trigger ogni frame.
        for (uint32_t i = 0; i < resp.state.count; i++) {
            const PlayerState& rp = resp.state.players[i];
            if (rp.player_id == 0 || rp.player_id == local_player_id_) continue;

            // Prima apparizione: inizializza prev state senza suonare.
            if (remote_prev_vel_y_.count(rp.player_id) == 0) {
                remote_prev_vel_y_[rp.player_id]          = rp.vel_y;
                remote_prev_dash_ticks_[rp.player_id]     = rp.dash_active_ticks;
                remote_prev_wall_jump_dir_[rp.player_id]  = rp.last_wall_jump_dir;
                remote_prev_checkpoint_[rp.player_id]     = {rp.checkpoint_x, rp.checkpoint_y};
                remote_prev_finished_[rp.player_id]       = rp.finished;
            }

            uint32_t& prev_tick = remote_last_ticks_[rp.player_id];
            if (rp.last_processed_tick != prev_tick) {
                prev_tick = rp.last_processed_tick;

                // Trail
                TrailState& rt = remote_trails_[rp.player_id];
                if (rp.dash_active_ticks > 0) rt.Push(rp.x, rp.y);
                else                          rt.Clear();

                // SFX spazializzato (solo player vivi e non in grace)
                uint8_t& prev_rdash = remote_prev_dash_ticks_[rp.player_id];
                float&   prev_rvy   = remote_prev_vel_y_[rp.player_id];
                int8_t&  prev_rwd   = remote_prev_wall_jump_dir_[rp.player_id];
                Vector2& prev_rcp   = remote_prev_checkpoint_[rp.player_id];

                if (rp.kill_respawn_ticks == 0 && rp.respawn_grace_ticks == 0) {
                    const float rcx = rp.x + TILE_SIZE * 0.5f;
                    const float rcy = rp.y + TILE_SIZE * 0.5f;
                    const float lcx = player_.GetState().x + TILE_SIZE * 0.5f;
                    const float lcy = player_.GetState().y + TILE_SIZE * 0.5f;

                    if (prev_rdash == 0 && rp.dash_active_ticks > 0)
                        sfx_.PlayDashAt(rcx, rcy, lcx, lcy);

                    // Wall jump prende priorità sul salto normale (stessa logica del locale).
                    const bool r_is_wj = (rp.last_wall_jump_dir != 0 && rp.last_wall_jump_dir != prev_rwd);
                    if (r_is_wj)
                        sfx_.PlayWallJumpAt(rcx, rcy, lcx, lcy);
                    else if (prev_rvy > -500.f && rp.vel_y <= -500.f)
                        sfx_.PlayJumpAt(rcx, rcy, lcx, lcy);

                    // Checkpoint
                    if (rp.checkpoint_x != prev_rcp.x || rp.checkpoint_y != prev_rcp.y)
                        sfx_.PlayCheckpointAt(rcx, rcy, lcx, lcy);

                    // Level complete
                    bool& prev_rfin = remote_prev_finished_[rp.player_id];
                    if (rp.finished && !prev_rfin)
                        sfx_.PlayLevelEndAt(rcx, rcy, lcx, lcy);
                    prev_rfin = rp.finished;
                } else {
                    // Player morto/in grace: resetta i prev per evitare falsi trigger al respawn.
                    prev_rdash = rp.dash_active_ticks;
                    prev_rvy   = rp.vel_y;
                    prev_rwd   = rp.last_wall_jump_dir;
                    prev_rcp   = {rp.checkpoint_x, rp.checkpoint_y};
                }

                prev_rdash = rp.dash_active_ticks;
                prev_rvy   = rp.vel_y;
                prev_rwd   = rp.last_wall_jump_dir;
                prev_rcp   = {rp.checkpoint_x, rp.checkpoint_y};
                remote_prev_finished_[rp.player_id] = rp.finished;
            }

            // --- Drawing trail (remote player) ---
            {
                auto& strokes = draw_trails_[rp.player_id];
                bool& prev_dr = draw_prev_drawing_[rp.player_id];
                if (rp.drawing && rp.kill_respawn_ticks == 0 && rp.respawn_grace_ticks == 0) {
                    int total = 0;
                    for (const auto& st : strokes) total += static_cast<int>(st.pts.size());
                    if (total < DRAW_MAX_POINTS) {
                        if (!prev_dr || strokes.empty()) {
                            strokes.push_back({});
                            strokes.back().created_time_s = GetTime();
                            strokes.back().pts.push_back({rp.x + TILE_SIZE * 0.5f,
                                                          rp.y + TILE_SIZE * 0.5f});
                        } else {
                            auto& cur = strokes.back().pts;
                            const Vector2 last = cur.back();
                            const float cx = rp.x + TILE_SIZE * 0.5f;
                            const float cy = rp.y + TILE_SIZE * 0.5f;
                            const float ddx = cx - last.x;
                            const float ddy = cy - last.y;
                            if (ddx * ddx + ddy * ddy >= DRAW_MIN_DIST * DRAW_MIN_DIST)
                                cur.push_back({cx, cy});
                        }
                    }
                }
                prev_dr = rp.drawing;
            }
        }

        // Reconciliation
        if (local_player_id_ != 0) {
            for (uint32_t i = 0; i < resp.state.count; i++) {
                const PlayerState& auth = resp.state.players[i];
                if (auth.player_id != local_player_id_) continue;

                const uint32_t srv_tick = auth.last_processed_tick;
                if (sim_tick_ > srv_tick && sim_tick_ - srv_tick < IHIST) {
                    player_.SetState(auth);
                    for (uint32_t t = srv_tick + 1; t < sim_tick_; t++) {
                        const InputFrame& hf = input_history_[t % IHIST];
                        if (hf.tick == t) player_.Simulate(hf, world_);
                    }
                } else if (sim_tick_ <= srv_tick) {
                    player_.SetState(auth);
                }
                break;
            }
        }
        return;
    }

    // PKT_GLOBAL_RESULTS: classifica vittorie di fine sessione
    if (pkt_type == PKT_GLOBAL_RESULTS && size >= sizeof(PktGlobalResults)) {
        PktGlobalResults gpkt{};
        std::memcpy(&gpkt, data, sizeof(gpkt));
        in_global_results_screen_    = true;
        local_global_ready_          = false;
        global_results_start_time_   = GetTime();
        global_results_count_        = gpkt.count;
        global_results_total_levels_ = gpkt.total_levels;
        global_results_coop_wins_    = gpkt.coop_wins;
        const int gn = std::min(static_cast<int>(gpkt.count), MAX_PLAYERS);
        for (int gi = 0; gi < gn; gi++) global_results_entries_[gi] = gpkt.entries[gi];
        return;
    }

    // PKT_LEVEL_RESULTS: mostra schermata risultati
    if (pkt_type == PKT_LEVEL_RESULTS && size >= sizeof(PktLevelResults)) {
        PktLevelResults rpkt{};
        std::memcpy(&rpkt, data, sizeof(rpkt));
        in_results_screen_          = true;
        local_ready_                = false;
        results_start_time_         = GetTime();
        results_count_              = rpkt.count;
        results_level_              = rpkt.level;
        results_coop_all_finished_  = (rpkt.coop_all_finished != 0);
        const int rn = std::min(static_cast<int>(rpkt.count), MAX_PLAYERS);
        for (int ri = 0; ri < rn; ri++) results_entries_[ri] = rpkt.entries[ri];
        return;
    }

    // PKT_LOAD_LEVEL: carica prossimo livello o termina partita
    if (pkt_type == PKT_LOAD_LEVEL && size >= sizeof(PktLoadLevel)) {
        PktLoadLevel lpkt{};
        std::memcpy(&lpkt, data, sizeof(lpkt));
        in_results_screen_        = false;
        in_global_results_screen_ = false;
        if (lpkt.is_last) {
            end_message_ = "Game over.";
            end_sub_msg_ = "Returning to main menu...";
            end_color_   = CLRS_SESSION_OK;
            session_over_ = true;
        } else {
            LoadLevel(lpkt.path);
        }
        return;
    }

    // PKT_LEVEL_DATA: generated level grid from server (chunk-based generator)
    if (pkt_type == PKT_LEVEL_DATA && size >= sizeof(PktLevelDataHeader)) {
        PktLevelDataHeader hdr{};
        std::memcpy(&hdr, data, sizeof(hdr));
        const size_t grid_size = static_cast<size_t>(hdr.width) * hdr.height;
        if (size >= sizeof(PktLevelDataHeader) + grid_size && hdr.width > 0 && hdr.height > 0) {
            in_results_screen_        = false;
            in_global_results_screen_ = false;
            if (hdr.is_last) {
                end_message_ = "Game over.";
                end_sub_msg_ = "Returning to main menu...";
                end_color_   = CLRS_SESSION_OK;
                session_over_ = true;
            } else {
                // Build rows from the char data that follows the header
                const char* tile_data = reinterpret_cast<const char*>(data + sizeof(PktLevelDataHeader));
                std::vector<std::string> rows(hdr.height);
                for (int y = 0; y < hdr.height; ++y)
                    rows[y].assign(tile_data + y * hdr.width, hdr.width);
                current_level_ = hdr.level;  // set before LoadLevelFromGrid so MakeLevelPalette sees the correct level
                LoadLevelFromGrid(hdr.width, hdr.height, rows);
                generating_level_ = false;  // level data received — hide loading overlay
            }
        }
        return;
    }

    // PKT_EMOTE_BROADCAST: remote player triggered an emote
    if (pkt_type == PKT_EMOTE_BROADCAST && size >= sizeof(PktEmoteBroadcast)) {
        PktEmoteBroadcast epkt{};
        std::memcpy(&epkt, data, sizeof(epkt));
        if (epkt.player_id != local_player_id_ && epkt.emote_id < EMOTE_COUNT) {
            EmoteBubble& eb = emote_bubbles_[epkt.player_id];
            eb.emote_id = epkt.emote_id;
            eb.timer    = EMOTE_DURATION;
            eb.active   = true;
            // Initialize position from current game state
            for (uint32_t i = 0; i < last_game_state_.count; ++i) {
                if (last_game_state_.players[i].player_id == epkt.player_id) {
                    eb.last_x = last_game_state_.players[i].x;
                    eb.last_y = last_game_state_.players[i].y;
                    break;
                }
            }
        }
        return;
    }

    // PKT_GENERATING: server is about to block its ENet loop for level generation.
    // Show a loading overlay and extend our timeout so we don't disconnect.
    if (pkt_type == PKT_GENERATING && size >= sizeof(PktGenerating)) {
        PktGenerating gpkt{};
        std::memcpy(&gpkt, data, sizeof(gpkt));
        generating_level_     = true;
        generating_level_num_ = gpkt.level;
        generating_elapsed_   = 0.f;
        net.SetLongTimeout(60000);  // 60 s — enough for any generation
        printf("[session] PKT_GENERATING level=%u\n", (unsigned)gpkt.level);
        return;
    }
}

// ---------------------------------------------------------------------------
// HandleDisconnect
// ---------------------------------------------------------------------------
void GameSession::HandleDisconnect(uint32_t disconnect_data) {
    if (!pending_disc_reason_.empty()) {
        end_message_ = pending_disc_reason_;
        end_sub_msg_ = pending_disc_sub_;
    } else if (disconnect_data == DISCONNECT_SERVER_BUSY) {
        end_message_ = "Server is busy. Try again later.";
        end_sub_msg_ = "A game is already in progress on this server.";
    } else if (disconnect_data == DISCONNECT_VERSION_MISMATCH) {
        end_message_ = "Version Mismatch: please update your client.";
    } else if (end_message_.empty()) {
        end_message_ = "Disconnected from server.";
    }
    session_over_ = true;
}

// ---------------------------------------------------------------------------
// LoadLevel — resetta lo stato effimero del livello corrente
// ---------------------------------------------------------------------------
void GameSession::LoadLevel(const char* path) {
    world_.LoadFromFile(path);

    // Lobby uses the default palette; any other file-based level also resets to default.
    palette_ = LevelPalette{};

    const bool loading_lobby = (std::strcmp(path, LOBBY_MAP_PATH) == 0);

    // Resetta il player allo spawn solo per i livelli reali, non per la lobby.
    if (!loading_lobby) {
        PlayerState new_ps{};
        new_ps.player_id = local_player_id_;
        std::strncpy(new_ps.name, username_.c_str(), sizeof(new_ps.name) - 1);
        const SpawnPos sp = FindCenterSpawn(world_);
        new_ps.x = sp.x;
        new_ps.y = sp.y;
        player_.SetState(new_ps);

        prev_x_ = new_ps.x;
        prev_y_ = new_ps.y;
        last_safe_x_ = new_ps.x;
        last_safe_y_ = new_ps.y;
    }

    sim_tick_    = 0;
    accumulator_ = 0.f;

    prev_finished_ = false;
    show_record_   = false;
    best_ticks_    = 0;

    trail_.Clear();
    remote_trails_.clear();
    remote_last_ticks_.clear();

    local_death_ = {};
    remote_deaths_.clear();
    remote_prev_kill_ticks_.clear();
    remote_last_alive_pos_.clear();
    remote_prev_dash_ticks_.clear();
    remote_prev_vel_y_.clear();
    remote_prev_wall_jump_dir_.clear();
    remote_prev_checkpoint_.clear();
    remote_prev_finished_.clear();
    prev_grabbed_state_.clear();
    prev_kill_ticks_local_ = 0;
    prev_grace_local_      = 0;
    prev_checkpoint_x_     = 0.f;
    prev_checkpoint_y_     = 0.f;

    live_best_ticks_.clear();
    last_game_state_ = {};

    draw_trails_.clear();
    draw_prev_drawing_.clear();

    std::memset(input_history_, 0, sizeof(input_history_));
}

// ---------------------------------------------------------------------------
// LoadLevelFromGrid — same as LoadLevel but from in-memory grid (generated levels)
// ---------------------------------------------------------------------------
void GameSession::LoadLevelFromGrid(int w, int h, const std::vector<std::string>& rows) {
    world_.LoadFromGrid(w, h, rows);

    // Generate a hue-rotated palette for this generated level.
    // current_level_ must be set before this call (done in HandlePacket).
    palette_ = MakeLevelPalette(current_level_);

    // Generated levels are never the lobby — always reset player state.
    PlayerState new_ps{};
    new_ps.player_id = local_player_id_;
    std::strncpy(new_ps.name, username_.c_str(), sizeof(new_ps.name) - 1);
    const SpawnPos sp = FindCenterSpawn(world_);
    new_ps.x = sp.x;
    new_ps.y = sp.y;
    player_.SetState(new_ps);

    prev_x_ = new_ps.x;
    prev_y_ = new_ps.y;
    last_safe_x_ = new_ps.x;
    last_safe_y_ = new_ps.y;

    sim_tick_    = 0;
    accumulator_ = 0.f;

    prev_finished_ = false;
    show_record_   = false;
    best_ticks_    = 0;

    trail_.Clear();
    remote_trails_.clear();
    remote_last_ticks_.clear();

    local_death_ = {};
    remote_deaths_.clear();
    remote_prev_kill_ticks_.clear();
    remote_last_alive_pos_.clear();
    remote_prev_dash_ticks_.clear();
    remote_prev_vel_y_.clear();
    remote_prev_wall_jump_dir_.clear();
    remote_prev_checkpoint_.clear();
    remote_prev_finished_.clear();
    prev_grabbed_state_.clear();
    prev_kill_ticks_local_ = 0;
    prev_grace_local_      = 0;
    prev_checkpoint_x_     = 0.f;
    prev_checkpoint_y_     = 0.f;

    live_best_ticks_.clear();
    last_game_state_ = {};

    draw_trails_.clear();
    draw_prev_drawing_.clear();

    std::memset(input_history_, 0, sizeof(input_history_));
}

// ---------------------------------------------------------------------------
// UpdateLiveBestTicks
// ---------------------------------------------------------------------------
void GameSession::UpdateLiveBestTicks() {
    // Dal game state autoritativo
    for (uint32_t i = 0; i < last_game_state_.count; i++) {
        const PlayerState& rp = last_game_state_.players[i];
        if (rp.player_id == 0 || !rp.finished || rp.level_ticks == 0) continue;
        auto it = live_best_ticks_.find(rp.player_id);
        if (it == live_best_ticks_.end() || rp.level_ticks < it->second)
            live_best_ticks_[rp.player_id] = rp.level_ticks;
    }
    // Dal player locale predetto
    const PlayerState& local = player_.GetState();
    if (local.player_id != 0 && local.finished && local.level_ticks > 0) {
        auto it = live_best_ticks_.find(local.player_id);
        if (it == live_best_ticks_.end() || local.level_ticks < it->second)
            live_best_ticks_[local.player_id] = local.level_ticks;
    }
}

// ---------------------------------------------------------------------------
// BuildLiveLeaderboard
// ---------------------------------------------------------------------------
void GameSession::BuildLiveLeaderboard(LiveLeaderEntry* out, int& count) const {
    count = 0;
    for (uint32_t i = 0; i < last_game_state_.count && count < MAX_PLAYERS; i++) {
        const PlayerState& rp = last_game_state_.players[i];
        if (rp.player_id == 0) continue;
        auto it = live_best_ticks_.find(rp.player_id);
        if (it == live_best_ticks_.end()) continue;
        LiveLeaderEntry& e = out[count++];
        e.player_id  = rp.player_id;
        e.best_ticks = it->second;
        std::strncpy(e.name, rp.name, 15);
        e.name[15] = '\0';
    }
    // Insertion sort (max 8 elementi)
    for (int a = 1; a < count; a++) {
        LiveLeaderEntry tmp = out[a];
        int b = a - 1;
        while (b >= 0 && out[b].best_ticks > tmp.best_ticks) { out[b + 1] = out[b]; b--; }
        out[b + 1] = tmp;
    }
}

// ---------------------------------------------------------------------------
// UpdateStrokeTessellation — incrementally tessellate Catmull-Rom spline.
// Only recomputes segments affected by newly-added control points.
// ---------------------------------------------------------------------------
void GameSession::UpdateStrokeTessellation(DrawStroke& st) {
    const int n = static_cast<int>(st.pts.size());
    if (n < 2) {
        st.tessellated.clear();
        st.tess_source_count = n;
        return;
    }
    if (n == st.tess_source_count) return; // nothing new

    if (n == 2) {
        st.tessellated = { st.pts[0], st.pts[1] };
        st.tess_source_count = n;
        return;
    }

    // With old_n control points the last segment's p3 was clamped; adding points
    // changes that p3, so segments 0..old_n-3 are stable.
    const int safe_segs = (st.tess_source_count >= 3) ? st.tess_source_count - 2 : 0;
    const int keep_verts = (safe_segs > 0) ? safe_segs * TESS_DIVISIONS + 1 : 0;
    if (keep_verts < static_cast<int>(st.tessellated.size()))
        st.tessellated.resize(keep_verts);

    const int total_segs = n - 1;
    st.tessellated.reserve(total_segs * TESS_DIVISIONS + 1);

    for (int seg = safe_segs; seg < total_segs; ++seg) {
        const Vector2 p0 = st.pts[std::max(seg - 1, 0)];
        const Vector2 p1 = st.pts[seg];
        const Vector2 p2 = st.pts[seg + 1];
        const Vector2 p3 = st.pts[std::min(seg + 2, n - 1)];

        if (st.tessellated.empty())
            st.tessellated.push_back(CatmullRomPoint(p0, p1, p2, p3, 0.f));

        for (int d = 1; d <= TESS_DIVISIONS; ++d) {
            const float t = static_cast<float>(d) / static_cast<float>(TESS_DIVISIONS);
            st.tessellated.push_back(CatmullRomPoint(p0, p1, p2, p3, t));
        }
    }
    st.tess_source_count = n;
}

// ---------------------------------------------------------------------------
// DoRender — tutto il frame di rendering
// ---------------------------------------------------------------------------
void GameSession::DoRender(float draw_x, float draw_y, float dt,
                            NetworkClient& net, Renderer& renderer) {
    (void)dt; // usato in futuro per effetti frame-interpolati

    renderer.SetPalette(palette_);
    renderer.BeginFrame();

    // Loading overlay: show while waiting for level data from server.
    // Drawn over everything else; skip all other world/HUD rendering while active.
    if (generating_level_) {
        renderer.DrawGeneratingLevel(generating_level_num_, generating_elapsed_);
        renderer.EndFrame();
        return;
    }
    renderer.BeginWorldDraw();

        renderer.DrawTilemap(world_);

        // --- Drawing trails (persistent map marks) ---
        {
            // Trails fade linearly and are dropped after DRAW_LIFETIME_S.
            const double now_s = GetTime();
            for (auto it = draw_trails_.begin(); it != draw_trails_.end(); ) {
                auto& strokes = it->second;
                strokes.erase(std::remove_if(strokes.begin(), strokes.end(), [&](const DrawStroke& st) {
                    return (now_s - st.created_time_s) >= static_cast<double>(DRAW_LIFETIME_S);
                }), strokes.end());

                if (strokes.empty()) {
                    draw_prev_drawing_.erase(it->first);
                    it = draw_trails_.erase(it);
                    continue;
                }

                const bool is_local_pid = (it->first == local_player_id_);
                const Color base_col = is_local_pid ? CLRS_PLAYER_LOCAL : CLRS_PLAYER_REMOTE;
                for (auto& st : strokes) {
                    const float age_s = static_cast<float>(now_s - st.created_time_s);
                    float fade = 1.f - (age_s / DRAW_LIFETIME_S);
                    if (fade <= 0.f) continue;
                    if (fade > 1.f) fade = 1.f;

                    const uint8_t alpha = static_cast<uint8_t>(200.f * fade);
                    if (alpha == 0) continue;
                    const Color col = {base_col.r, base_col.g, base_col.b, alpha};

                    UpdateStrokeTessellation(st);
                    Renderer::DrawStroke draw_stroke{};
                    if (st.pts.size() == 1) {
                        draw_stroke = {st.pts.data(), 1};
                    } else if (!st.tessellated.empty()) {
                        draw_stroke = {st.tessellated.data(), static_cast<int>(st.tessellated.size())};
                    }
                    if (draw_stroke.count > 0)
                        renderer.DrawMapTrails(&draw_stroke, 1, col);
                }
                ++it;
            }
        }

        // Trail remoti
        for (uint32_t i = 0; i < last_game_state_.count; i++) {
            const PlayerState& rp = last_game_state_.players[i];
            if (rp.player_id == 0 || rp.player_id == local_player_id_) continue;
            renderer.DrawTrail(remote_trails_[rp.player_id], false);
        }
        renderer.DrawTrail(trail_, true);

        // Particelle di morte
        for (auto& [id, dp] : remote_deaths_) renderer.DrawDeathParticles(dp);
        renderer.DrawDeathParticles(local_death_);

        // Player remoti (posizione autoritativa)
        if (local_player_id_ != 0) {
            for (uint32_t i = 0; i < last_game_state_.count; i++) {
                const PlayerState& rp = last_game_state_.players[i];
                if (rp.player_id != 0 && rp.player_id != local_player_id_
                    && rp.kill_respawn_ticks == 0)
                    renderer.DrawPlayer(rp.x, rp.y, rp, false,
                                        rp.player_id == last_game_state_.leader_id);
            }
        }

        // Player locale (nascosto durante il respawn)
        const PlayerState& local = player_.GetState();
        if (local.kill_respawn_ticks == 0)
            renderer.DrawPlayer(draw_x, draw_y, local, true,
                                local.player_id == last_game_state_.leader_id);

        // Grab marker: colored midpoint circle between grabber and grabbed player.
        {
            const auto marker_color_for = [](const PlayerState& s, bool is_local) {
                const bool bright = (s.dash_active_ticks > 0 || s.dash_ready);
                if (is_local) return bright ? CLRS_PLAYER_LOCAL : CLRS_PLAYER_LOCAL_DIM;
                return bright ? CLRS_PLAYER_REMOTE : CLRS_PLAYER_REMOTE_DIM;
            };
            const float max_dist2 = (TILE_SIZE * 1.5f) * (TILE_SIZE * 1.5f);
            for (uint32_t gi = 0; gi < last_game_state_.count; ++gi) {
                const PlayerState& grabbed = last_game_state_.players[gi];
                if (grabbed.player_id == 0 || !grabbed.grabbed) continue;
                if (grabbed.kill_respawn_ticks > 0 || grabbed.respawn_grace_ticks > 0) continue;

                int best_idx = -1;
                float best_d2 = max_dist2;
                for (uint32_t ci = 0; ci < last_game_state_.count; ++ci) {
                    const PlayerState& cand = last_game_state_.players[ci];
                    if (cand.player_id == 0 || cand.player_id == grabbed.player_id) continue;
                    if (!cand.magneting || cand.grabbed) continue;
                    if (cand.kill_respawn_ticks > 0 || cand.respawn_grace_ticks > 0) continue;

                    const float dx = grabbed.x - cand.x;
                    const float dy = grabbed.y - (cand.y - static_cast<float>(TILE_SIZE));
                    const float d2 = dx * dx + dy * dy;
                    if (d2 < best_d2) {
                        best_d2 = d2;
                        best_idx = static_cast<int>(ci);
                    }
                }

                if (best_idx < 0) continue;
                const PlayerState& grabber = last_game_state_.players[best_idx];
                const bool is_local_grabber = (grabber.player_id == local_player_id_);
                const Color marker_col = marker_color_for(grabber, is_local_grabber);
                const float ax = grabber.x + TILE_SIZE * 0.5f;
                const float ay = grabber.y + TILE_SIZE * 0.5f;
                const float bx = grabbed.x + TILE_SIZE * 0.5f;
                const float by = grabbed.y + TILE_SIZE * 0.5f;
                renderer.DrawGrabLinkMarker(ax, ay, bx, by, marker_col, TILE_SIZE * 0.25f);
            }
        }

        // --- Emote bubbles (world-space, above each player) ---
        for (auto& [pid, eb] : emote_bubbles_) {
            if (!eb.active) continue;
            eb.timer -= dt;
            if (eb.timer <= 0.f) { eb.active = false; continue; }
            // Alpha: fade in for first 0.2s, fade out for last 0.5s, full in between
            float alpha = 1.f;
            const float elapsed = EMOTE_DURATION - eb.timer;
            if (elapsed < 0.2f)      alpha = elapsed / 0.2f;
            if (eb.timer < 0.5f)     alpha = std::min(alpha, eb.timer / 0.5f);
            // Update position only while alive (freeze on death/respawn)
            if (pid == local_player_id_) {
                if (local.kill_respawn_ticks == 0 && local.respawn_grace_ticks == 0) {
                    eb.last_x = draw_x;
                    eb.last_y = draw_y;
                }
            } else {
                for (uint32_t i = 0; i < last_game_state_.count; ++i) {
                    const auto& rp = last_game_state_.players[i];
                    if (rp.player_id == pid) {
                        if (rp.kill_respawn_ticks == 0 && rp.respawn_grace_ticks == 0) {
                            eb.last_x = rp.x;
                            eb.last_y = rp.y;
                        }
                        break;
                    }
                }
            }
            renderer.DrawEmoteBubble(eb.last_x, eb.last_y - 24.f, eb.emote_id, alpha,
                                     pid == local_player_id_);
        }

    renderer.EndWorldDraw();

    // Off-screen player indicators (below HUD, above world)
    if (local_player_id_ != 0 && last_game_state_.count > 1)
        renderer.DrawOffscreenArrows(last_game_state_, local_player_id_);

    // HUD
    const GameMode cur_mode = static_cast<GameMode>(last_game_state_.game_mode);
    renderer.DrawHUD(local, last_game_state_.count, !is_offline_, cur_mode);
    renderer.DrawLevelIndicator(current_level_);
    renderer.DrawNetStats(net.GetRTT(), net.GetJitter(), net.GetLoss());

    // Classifica live (solo durante il gioco normale, non in lobby)
    if (!last_game_state_.is_lobby && last_game_state_.count > 0) {
        LiveLeaderEntry entries[MAX_PLAYERS];
        int entry_count = 0;
        BuildLiveLeaderboard(entries, entry_count);
        renderer.DrawLiveLeaderboard(entries, entry_count);
    }

    // Overlay "Ready?" / "Go!"
    renderer.DrawReadyGo(local.respawn_grace_ticks, dt);

#ifndef NDEBUG
    renderer.DrawDebugPanel(local);
#endif

    // Timer (nascosto in lobby)
    if (!last_game_state_.is_lobby) {
        renderer.DrawTimer(local, best_ticks_,
            last_game_state_.time_limit_secs,
            last_game_state_.next_level_countdown_ticks,
            cur_mode);
    }

    if (last_game_state_.is_lobby) {
        renderer.DrawLobbyHints(last_game_state_.next_level_countdown_ticks,
                                last_game_state_.count);
        // Lobby options panel
        const bool am_leader = (local_player_id_ == last_game_state_.leader_id);
        renderer.DrawLobbyOptions(cur_mode, am_leader,
                                  last_game_state_.leader_id, last_game_state_);
    }

    // Emote wheel (screen-space, centered, above HUD, below pause)
    if (input_sampler_.IsEmoteWheelOpen() && pause_state_ == PauseState::PLAYING) {
        const float sw = static_cast<float>(GetScreenWidth());
        const float sh = static_cast<float>(GetScreenHeight());
        renderer.DrawEmoteWheel(sw * 0.5f, sh * 0.5f, input_sampler_.GetEmoteWheelHighlight());
    }

    const bool show_lobby_settings = last_game_state_.is_lobby
        && (local_player_id_ == last_game_state_.leader_id);
    uint8_t lobby_max_levels = last_game_state_.max_generated_levels;
    if (lobby_max_levels < static_cast<uint8_t>(MIN_GENERATED_LEVELS) ||
        lobby_max_levels > static_cast<uint8_t>(MAX_GENERATED_LEVELS_LIMIT)) {
        lobby_max_levels = static_cast<uint8_t>(MAX_GENERATED_LEVELS);
    }
    renderer.DrawPauseMenu(pause_state_, pause_focused_, confirm_focused_, sfx_.IsMuted(),
                           show_lobby_settings, cur_mode, lobby_max_levels);

    renderer.DrawResultsScreen(in_results_screen_, local_ready_,
        results_entries_, results_count_, results_level_,
        GetTime() - results_start_time_, RESULTS_DURATION_S,
        results_coop_all_finished_, cur_mode);

    renderer.DrawGlobalResultsScreen(in_global_results_screen_, local_global_ready_,
        global_results_entries_, global_results_count_, global_results_total_levels_,
        GetTime() - global_results_start_time_, GLOBAL_RESULTS_DURATION_S,
        global_results_coop_wins_, cur_mode);

    renderer.EndFrame();
}
