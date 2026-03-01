// PASSO 1: finestra vuota con FPS                                    — COMPLETATO
// PASSO 2: tilemap da file, tutto in main                            — COMPLETATO
// PASSO 3: classe World in common                                    — COMPLETATO
// PASSO 4: PlayerState + Player, rettangolo che si muove             — COMPLETATO
// PASSO 5: gravità, vel_y, salto (senza collisioni)                 — COMPLETATO
// PASSO 6: collisioni X/Y, on_ground, jump buffer, coyote, cut, WJ  — COMPLETATO
// PASSO 7: fixed timestep 60Hz, accumulator                         — COMPLETATO
// PASSO 8: Camera2D smooth follow, world-space render               — COMPLETATO
// PASSO 9: InputFrame + Player::Simulate, unico punto deterministico — COMPLETATO
// PASSO 10: Sticky flag per jump e dash                               — COMPLETATO
// PASSO 13: Client invia InputFrame al server, riceve PlayerState     — COMPLETATO
// PASSO 14: GameState multi-giocatore, rendering remoti               — COMPLETATO
// PASSO 15: Client-side prediction                                    — COMPLETATO
// PASSO 16: Reconciliation (reset + replay dal tick autoritativo)     — COMPLETATO
// PASSO 19: Menu iniziale (Offline/Online, nome player)              — COMPLETATO
// PASSO 20: Modalità offline (LocalServer in thread background)      — COMPLETATO

#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include <raylib.h>
#include "World.h"
#include "Player.h"
#include "Physics.h"
#include "InputFrame.h"
#include <enet/enet.h>
#include "Protocol.h"
#include "MainMenu.h"
#include "LocalServer.h"
#include "SaveData.h"
#include "WinIcon.h"

// ---------------------------------------------------------------------------
// Costanti gamepad (layout PS)
// ---------------------------------------------------------------------------
static constexpr int   GP          = 0;
static constexpr float GP_DEADZONE = 0.25f;

static constexpr GamepadButton GP_JUMP    = GAMEPAD_BUTTON_RIGHT_FACE_DOWN;   // Croce
static constexpr GamepadButton GP_DASH_A  = GAMEPAD_BUTTON_RIGHT_FACE_LEFT;   // Quadrato
static constexpr GamepadButton GP_DASH_B  = GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;  // Cerchio
static constexpr GamepadButton GP_RESTART = GAMEPAD_BUTTON_RIGHT_FACE_UP;     // Triangolo
static constexpr GamepadButton GP_START   = GAMEPAD_BUTTON_MIDDLE_RIGHT;       // Options / Start

// Fattore di smorzamento della camera (valori alti = segue più stretto).
static constexpr float CAM_SMOOTH  = 8.f;

// ---------------------------------------------------------------------------
// Helpers input
// ---------------------------------------------------------------------------

// Componente X discreta dello stick/dpad gamepad.
static float GpAxisX() {
    if (!IsGamepadAvailable(GP)) return 0.f;
    const float a = GetGamepadAxisMovement(GP, GAMEPAD_AXIS_LEFT_X);
    if (a >  GP_DEADZONE) return  1.f;
    if (a < -GP_DEADZONE) return -1.f;
    if (IsGamepadButtonDown(GP, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) return  1.f;
    if (IsGamepadButtonDown(GP, GAMEPAD_BUTTON_LEFT_FACE_LEFT))  return -1.f;
    return 0.f;
}

// Campiona il vettore grezzo di direzione (tastiera + gamepad) per dash/steer.
// Il gamepad analogico sovrascrive la tastiera se lo stick è fuori dalla deadzone.
static void SampleDashDir(bool gp, float& out_x, float& out_y) {
    out_x = 0.f; out_y = 0.f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) out_x += 1.f;
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) out_x -= 1.f;
    if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) out_y += 1.f;
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) out_y -= 1.f;
    if (gp) {
        const float ax = GetGamepadAxisMovement(GP, GAMEPAD_AXIS_LEFT_X);
        const float ay = GetGamepadAxisMovement(GP, GAMEPAD_AXIS_LEFT_Y);
        if (ax * ax + ay * ay > GP_DEADZONE * GP_DEADZONE) {
            out_x = ax; out_y = ay;
        } else {
            if (IsGamepadButtonDown(GP, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) out_x += 1.f;
            if (IsGamepadButtonDown(GP, GAMEPAD_BUTTON_LEFT_FACE_LEFT))  out_x -= 1.f;
            if (IsGamepadButtonDown(GP, GAMEPAD_BUTTON_LEFT_FACE_DOWN))  out_y += 1.f;
            if (IsGamepadButtonDown(GP, GAMEPAD_BUTTON_LEFT_FACE_UP))    out_y -= 1.f;
        }
    }
}

// ---------------------------------------------------------------------------
// Trail del dash (client-side, puramente visuale)
// ---------------------------------------------------------------------------
struct TrailState {
    struct Point { float x, y; };
    static constexpr int LEN = 12;
    Point pts[LEN] = {};
    int   count    = 0;

    void Push(float x, float y) {
        for (int i = LEN - 1; i > 0; i--) pts[i] = pts[i - 1];
        pts[0] = {x, y};
        if (count < LEN) count++;
    }
    void Clear() { count = 0; }

    void Draw(Color base_col) const {
        for (int i = count - 1; i >= 0; i--) {
            const float   t     = 1.f - static_cast<float>(i + 1) / static_cast<float>(LEN + 1);
            const uint8_t alpha = static_cast<uint8_t>(t * 80.f);
            const int     sh    = i / 2;
            DrawRectangle(
                static_cast<int>(pts[i].x) + sh / 2,
                static_cast<int>(pts[i].y) + sh / 2,
                TILE_SIZE - sh, TILE_SIZE - sh,
                {base_col.r, base_col.g, base_col.b, alpha});
        }
    }
};

// ---------------------------------------------------------------------------
// Rendering helpers (world-space, chiamati dentro BeginMode2D)
// ---------------------------------------------------------------------------

static void DrawTilemap(const World& world) {
    for (int ty = 0; ty < world.GetHeight(); ty++) {
        for (int tx = 0; tx < world.GetWidth(); tx++) {
            const char c = world.GetTile(tx, ty);
            Color col;
            if      (c == '0') col = { 60, 100, 180, 255};  // muro
            else if (c == 'E') col = {  0, 230, 100, 255};  // endpoint
#ifndef NDEBUG
            else if (c == 'X') col = { 60,  80, 220, 255};  // spawn (solo debug)
#endif
            else continue;
            DrawRectangle(tx * TILE_SIZE, ty * TILE_SIZE, TILE_SIZE, TILE_SIZE, col);
        }
    }
}

static void DrawPlayer(Font& font, float rx, float ry,
                       const PlayerState& s, bool is_local = true) {
    // Corpo del player.
    Color col;
    // Bright = dash disponibile OPPURE dash attivo in quel momento.
    // Scuro = solo durante il cooldown (dash esaurito, non ancora ricaricato).
    if (is_local) {
        const bool bright = s.dash_active_ticks > 0 || s.dash_cooldown_ticks == 0;
        col = bright ? Color{0, 220, 255, 255} : Color{0, 90, 115, 255};
    } else {
        const bool bright = s.dash_active_ticks > 0 || s.dash_cooldown_ticks == 0;
        col = bright ? Color{255, 140, 60, 200} : Color{140, 80, 40, 160};
    }
    DrawRectangle(static_cast<int>(rx), static_cast<int>(ry),
                  TILE_SIZE, TILE_SIZE, col);

    // Nome sopra il rettangolo (world-space, inside BeginMode2D) — solo per i remoti.
    if (!is_local && s.name[0] != '\0') {
        const float   nm_sz = 24.f;
        const Vector2 nm_ts = MeasureTextEx(font, s.name, nm_sz, 1);
        DrawTextEx(font, s.name,
            {rx + TILE_SIZE * 0.5f - nm_ts.x * 0.5f, ry - nm_sz - 4},
            nm_sz, 1,
            is_local ? Color{180, 240, 255, 220} : Color{255, 210, 150, 200});
    }
}

// ---------------------------------------------------------------------------
// Rendering helpers (screen-space, chiamati fuori da BeginMode2D)
// ---------------------------------------------------------------------------

static void DrawHUD(Font& font, const PlayerState& s, uint32_t player_count, bool show_players = true) {
    (void)s;
    const char* txt = show_players
        ? TextFormat("fps: %d  players: %u", GetFPS(), player_count)
        : TextFormat("fps: %d", GetFPS());
    DrawTextEx(font, txt, {10, 10}, 24, 1, WHITE);
}

static void DrawDebugPanel(Font& font, const PlayerState& s) {
    const float y0 = static_cast<float>(GetScreenHeight() - 18 * 12 - 10);
    DrawTextEx(font,
        TextFormat(
            "x:             %.0f\n"
            "y:             %.0f\n"
            "on_ground:     %s\n"
            "on_wall_left:  %s\n"
            "on_wall_right: %s\n"
            "last_wj_dir:   %d\n"
            "jump_buf:      %d\n"
            "coyote:        %d\n"
            "vel_x:         %.0f\n"
            "vel_y:         %.0f\n"
            "dash_active:   %d\n"
            "dash_cooldown: %d",
            s.x, s.y,
            s.on_ground     ? "YES" : "no",
            s.on_wall_left  ? "YES" : "no",
            s.on_wall_right ? "YES" : "no",
            static_cast<int>(s.last_wall_jump_dir),
            static_cast<int>(s.jump_buffer_ticks),
            static_cast<int>(s.coyote_ticks),
            s.vel_x, s.vel_y,
            static_cast<int>(s.dash_active_ticks),
            static_cast<int>(s.dash_cooldown_ticks)
        ),
        {10, y0}, 18, 1, {200, 200, 200, 200});
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    if (enet_initialize() != 0) {
        fprintf(stderr, "[client] ERRORE: enet_initialize fallita\n");
        return 1;
    }
    printf("[client] TileRace v%s  (protocol %u)\n", GAME_VERSION, PROTOCOL_VERSION);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, TextFormat("TileRace v%s", GAME_VERSION));
    // Imposta l'icona sulla finestra Win32 nativa: carica dalla risorsa embedded
    // (ID 1 nel .rc), poi la forza sia come ICON_SMALL che ICON_BIG in modo che
    // la taskbar e alt+tab la mostrino correttamente anche dopo che GLFW ha
    // inizializzato la finestra con la sua icona di default.
    ApplyExeIconToWindow(GetWindowHandle());
    SetTargetFPS(120);
    SetExitKey(KEY_NULL);  // ESC gestito manualmente (pausa in-game, quit da menu)

    Font font_small = LoadFontEx("assets/fonts/SpaceMono-Regular.ttf", 18, nullptr, 0);
    Font font_hud   = LoadFontEx("assets/fonts/SpaceMono-Regular.ttf", 24, nullptr, 0);
    Font font_timer = LoadFontEx("assets/fonts/SpaceMono-Regular.ttf", 48, nullptr, 0);
    SetTextureFilter(font_timer.texture, TEXTURE_FILTER_BILINEAR);

    // Carica i dati salvati (nickname, ultimo IP).
    SaveData save{};
    LoadSaveData(save);

    // -----------------------------------------------------------------------
    // Loop principale: menu --> partita --> menu (riparte se la connessione fallisce)
    // -----------------------------------------------------------------------
    while (!WindowShouldClose()) {

    // Menu iniziale (passo 19)
    const MenuResult menu = ShowMainMenu(font_hud, save);
    if (menu.choice == MenuChoice::QUIT) break;

    std::string session_error;                               // non-empty = mostra messaggio prima di tornare al menu
    std::string session_error_sub;                          // riga di dettaglio opzionale
    Color       session_error_color = {255, 80, 80, 255};  // rosso = errore, verde = fine partita

    // Nella modalità offline il server locale parte direttamente dal primo livello,
    // bypassando la lobby (che è pensata solo per il multiplayer online).
    LocalServer local_srv;
    const bool is_offline = (menu.choice == MenuChoice::OFFLINE);
    const char* initial_map = is_offline
        ? "assets/levels/level_01.txt"
        : LOBBY_MAP_PATH;
    if (is_offline)
        local_srv.Start(SERVER_PORT_LOCAL, initial_map);

    const char* connect_ip = is_offline ? "127.0.0.1" : menu.server_ip;
    const uint16_t connect_port = is_offline ? SERVER_PORT_LOCAL : SERVER_PORT;

    // Mondo e spawn
    World world;
    world.LoadFromFile(initial_map);

    PlayerState ps;
    ps.player_id = 0;   // assegnato dal server via PktWelcome
    std::strncpy(ps.name, menu.username, sizeof(ps.name) - 1);
    ps.name[sizeof(ps.name) - 1] = '\0';
    for (int ty = 0; ty < world.GetHeight(); ty++)
        for (int tx = 0; tx < world.GetWidth(); tx++)
            if (world.GetTile(tx, ty) == 'X') {
                ps.x = static_cast<float>(tx * TILE_SIZE);
                ps.y = static_cast<float>(ty * TILE_SIZE);
            }

    // Stato predetto localmente (client-side prediction, passo 15).
    Player player;
    player.SetState(ps);

    // ID assegnato dal server al momento della connessione (0 = non ancora ricevuto).
    uint32_t local_player_id = 0;

    // Ring buffer degli InputFrame già inviati — usato per la reconciliation (passo 16).
    static constexpr uint32_t IHIST = 128;
    InputFrame input_history[IHIST] = {};

    // Ultimo GameState ricevuto: usato per renderare gli altri giocatori.
    GameState last_game_state{};

    // --- Connessione ENet al server ---
    ENetHost* net_host = enet_host_create(nullptr, 1, CHANNEL_COUNT, 0, 0);
    ENetAddress net_addr{};
    enet_address_set_host(&net_addr, connect_ip);
    net_addr.port = connect_port;
    ENetPeer* net_peer = enet_host_connect(net_host, &net_addr, CHANNEL_COUNT, 0);
    {
        bool connected = false;
        printf("[client] connessione a %s:%u in corso...\n", connect_ip, connect_port);
        for (int i = 0; i < 60 && !connected; ++i) {
            ENetEvent ev;
            if (enet_host_service(net_host, &ev, 50) > 0 &&
                ev.type == ENET_EVENT_TYPE_CONNECT) {
                connected = true;
                printf("[client] connesso al server\n");
            }
        }
        if (!connected) {
            fprintf(stderr, "[client] server non raggiungibile\n");
            enet_peer_reset(net_peer);
            enet_host_destroy(net_host);
            local_srv.Stop();
            // Mostra errore per 2 secondi poi torna al menu.
            const double t0 = GetTime();
            while (!WindowShouldClose() && GetTime() - t0 < 2.0) {
                BeginDrawing();
                ClearBackground({8, 12, 40, 255});
                const char* msg = "Connection failed. Check IP and port.";
                const Vector2 ms = MeasureTextEx(font_hud, msg, 24, 1);
                DrawTextEx(font_hud, msg,
                    {GetScreenWidth() * 0.5f - ms.x * 0.5f,
                     GetScreenHeight() * 0.5f - 12.f},
                    24, 1, {255, 80, 80, 255});
                EndDrawing();
            }
            continue;
        }
    }

    TrailState trail;
    std::unordered_map<uint32_t, TrailState>  remote_trails;      // key = player_id
    std::unordered_map<uint32_t, uint32_t>    remote_last_ticks;   // key = player_id

    // Camera con smooth follow; il target insegue il player con lerp esponenziale.
    Camera2D camera{};
    camera.offset   = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
    camera.target   = {ps.x + TILE_SIZE * 0.5f,  ps.y + TILE_SIZE * 0.5f};
    camera.rotation = 0.f;
    camera.zoom     = 1.f;

    float    accumulator    = 0.f;
    uint32_t sim_tick       = 0;
    bool     jump_pressed   = false;  // edge rising sticky fino al prossimo tick fisso
    bool     dash_pending   = false;
    bool     restart_pending = false;
    // Record e stato di completamento livello.
    bool     prev_finished  = false;
    uint32_t best_ticks     = 0;      // 0 = nessun record ancora
    bool     show_record    = false;
    // Posizione al tick precedente, per interpolazione visiva.
    float    prev_x         = ps.x;
    float    prev_y         = ps.y;

    bool session_over = false;
    // Ragione di disconnessione ricevuta via PKT_VERSION_MISMATCH prima del DISCONNECT.
    std::string pending_disc_reason;
    std::string pending_disc_sub;
    enum class PauseState { PLAYING, PAUSED, CONFIRM_QUIT };
    float prev_pause_stick_y = 0.f;  // edge detection stick per navigazione pausa
    PauseState pause_state     = PauseState::PLAYING;
    int        pause_focused   = 0;  // 0 = Resume, 1 = Quit to Menu
    int        confirm_focused = 0;  // 0 = No, 1 = Yes
    while (!WindowShouldClose() && !session_over) {
        const float dt = GetFrameTime();
        accumulator += dt;

        // --- Input sticky (va campionato PRIMA di BeginDrawing) ---
        const bool gp = IsGamepadAvailable(GP);

        // --- Pausa / resume ---
        {
            const bool toggle   = IsKeyPressed(KEY_ESCAPE) ||
                                  (gp && IsGamepadButtonPressed(GP, GP_START));
            // Stick edge detection per navigazione pausa
            const float gp_sy = gp ? GetGamepadAxisMovement(GP, GAMEPAD_AXIS_LEFT_Y) : 0.f;
            const bool stick_nav_up   = (prev_pause_stick_y > -0.5f && gp_sy <= -0.5f);
            const bool stick_nav_down = (prev_pause_stick_y <  0.5f && gp_sy >=  0.5f);
            prev_pause_stick_y = gp_sy;
            // Nav: su / sinistra  (frecce, WASD, D-pad, stick)
            const bool nav_up   = IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W) ||
                                  IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A) ||
                                  (gp && (IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_LEFT_FACE_UP) ||
                                          IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_LEFT_FACE_LEFT))) ||
                                  stick_nav_up;
            // Nav: giù / destra
            const bool nav_down = IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S) ||
                                  IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) ||
                                  (gp && (IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_LEFT_FACE_DOWN) ||
                                          IsGamepadButtonPressed(GP, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))) ||
                                  stick_nav_down;
            // Conferma: Enter, Space (tasto salta) o Cross
            const bool ok       = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
                                  (gp && IsGamepadButtonPressed(GP, GP_JUMP));

            // Helper: rettangolo cliccabile centrato per la voce i (stessa geometria del render).
            const float pcx = GetScreenWidth()  * 0.5f;
            const float pcy = GetScreenHeight() * 0.5f;
            auto item_rect = [&](int i) -> Rectangle {
                const float item_y = pcy - 10.f + i * 44.f;
                const Vector2 sz = MeasureTextEx(font_hud, "  Quit to Menu", 24, 1); // voce più lunga
                return Rectangle{pcx - sz.x * 0.5f - 8, item_y - 4, sz.x + 16, 24.f + 8};
            };
            const Vector2 mouse    = GetMousePosition();
            const bool    clicked  = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

            if (pause_state == PauseState::PLAYING && toggle) {
                pause_state   = PauseState::PAUSED;
                pause_focused = 0;
            } else if (pause_state == PauseState::PAUSED) {
                // Hover mouse → aggiorna focus
                for (int i = 0; i < 2; i++)
                    if (CheckCollisionPointRec(mouse, item_rect(i))) pause_focused = i;
                if      (toggle)             pause_state   = PauseState::PLAYING;
                else if (nav_up || nav_down) pause_focused = 1 - pause_focused;
                else if (ok || (clicked && CheckCollisionPointRec(mouse, item_rect(pause_focused)))) {
                    if (pause_focused == 0)  pause_state   = PauseState::PLAYING;
                    else { pause_state = PauseState::CONFIRM_QUIT; confirm_focused = 0; }
                }
            } else if (pause_state == PauseState::CONFIRM_QUIT) {
                // Hover mouse → aggiorna focus
                for (int i = 0; i < 2; i++)
                    if (CheckCollisionPointRec(mouse, item_rect(i))) confirm_focused = i;
                if      (toggle)             pause_state     = PauseState::PAUSED;
                else if (nav_up || nav_down) confirm_focused = 1 - confirm_focused;
                else if (ok || (clicked && CheckCollisionPointRec(mouse, item_rect(confirm_focused)))) {
                    if (confirm_focused == 1) session_over = true;
                    else                      pause_state  = PauseState::PAUSED;
                }
            }
        }
        if (session_over) continue;

        if (pause_state == PauseState::PLAYING &&
            (IsKeyPressed(KEY_SPACE) || (gp && IsGamepadButtonPressed(GP, GP_JUMP))))
            jump_pressed = true;

        if (pause_state == PauseState::PLAYING &&
            (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT) ||
            (gp && (IsGamepadButtonPressed(GP, GP_DASH_A) ||
                    IsGamepadButtonPressed(GP, GP_DASH_B)))))
            dash_pending = true;

        if (pause_state == PauseState::PLAYING &&
            (IsKeyPressed(KEY_BACKSPACE) ||
            (gp && IsGamepadButtonPressed(GP, GP_RESTART))))
            restart_pending = true;

        // --- Restart: invia pacchetto e azzera stato record ---
        if (restart_pending) {
            PktRestart rpkt{};
            enet_peer_send(net_peer, CHANNEL_RELIABLE,
                enet_packet_create(&rpkt, sizeof(rpkt), ENET_PACKET_FLAG_RELIABLE));
            restart_pending = false;
            show_record     = false;
        }

        // --- Tick fisso 60 Hz ---
        if (pause_state != PauseState::PLAYING) accumulator = 0.f;
        while (accumulator >= FIXED_DT) {
            // Trail (usa stato locale predetto)
            if (player.GetState().dash_active_ticks > 0)
                trail.Push(player.GetState().x, player.GetState().y);
            else
                trail.Clear();

            // Costruisci InputFrame
            const bool  jump_held = IsKeyDown(KEY_SPACE) || (gp && IsGamepadButtonDown(GP, GP_JUMP));
            float dash_rx, dash_ry;
            SampleDashDir(gp, dash_rx, dash_ry);

            // Asse X di movimento: tastiera/DPAD = ±1, stick analogico = valore grezzo.
            float move_x = 0.f;
            if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) move_x -= 1.f;
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) move_x += 1.f;
            if (gp) {
                const float ax = GetGamepadAxisMovement(GP, GAMEPAD_AXIS_LEFT_X);
                if (fabsf(ax) > GP_DEADZONE) {
                    move_x = ax;  // sovrascrive tastiera con valore analogico
                } else if (fabsf(move_x) < 0.01f) {
                    if (IsGamepadButtonDown(GP, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) move_x += 1.f;
                    if (IsGamepadButtonDown(GP, GAMEPAD_BUTTON_LEFT_FACE_LEFT))  move_x -= 1.f;
                }
            }
            move_x = fmaxf(-1.f, fminf(1.f, move_x));

            InputFrame frame;
            frame.tick    = sim_tick++;
            frame.dash_dx = dash_rx;
            frame.dash_dy = dash_ry;
            frame.move_x  = move_x;
            if (move_x < -0.01f) frame.buttons |= BTN_LEFT;
            if (move_x >  0.01f) frame.buttons |= BTN_RIGHT;
            if (jump_held)    frame.buttons |= BTN_JUMP;
            if (jump_pressed) { frame.buttons |= BTN_JUMP_PRESS; jump_pressed = false; }
            if (dash_pending) { frame.buttons |= BTN_DASH;       dash_pending = false; }

            // Invia input al server.
            PktInput pkt{};
            pkt.frame = frame;
            ENetPacket* out = enet_packet_create(&pkt, sizeof(pkt), 0);
            enet_peer_send(net_peer, CHANNEL_RELIABLE, out);

            // Archivia nella storia per la reconciliation.
            input_history[frame.tick % IHIST] = frame;

            // Simulazione locale (prediction): salva pos precedente poi avanza.
            prev_x = player.GetState().x;
            prev_y = player.GetState().y;
            player.Simulate(frame, world);

            accumulator -= FIXED_DT;
        }

        // --- Ricezione stato dal server ---
        {
            ENetEvent ev;
            while (enet_host_service(net_host, &ev, 0) > 0) {
                if (ev.type == ENET_EVENT_TYPE_RECEIVE &&
                    ev.packet->dataLength >= 1) {
                    const uint8_t pkt_type = ev.packet->data[0];

                    if (pkt_type == PKT_WELCOME &&
                        ev.packet->dataLength >= sizeof(PktWelcome)) {
                        // Riceve player_id, imposta nome, invia PKT_PLAYER_INFO.
                        PktWelcome welcome{};
                        std::memcpy(&welcome, ev.packet->data, sizeof(PktWelcome));
                        local_player_id = welcome.player_id;
                        printf("[client] player_id = %u  session_token = %u\n",
                               welcome.player_id, welcome.session_token);
                        PlayerState s   = player.GetState();
                        s.player_id     = local_player_id;
                        player.SetState(s);

                        // Invia nome + versione al server.
                        PktPlayerInfo info{};
                        info.protocol_version = PROTOCOL_VERSION;
                        std::strncpy(info.name, menu.username, sizeof(info.name) - 1);
                        ENetPacket* pi = enet_packet_create(&info, sizeof(info),
                                                            ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(net_peer, CHANNEL_RELIABLE, pi);
                        printf("[client] player_id = %u  nome = '%s'  protocol_version = %u\n",
                               local_player_id, menu.username, PROTOCOL_VERSION);
                    }
                    // Versione protocollo incompatibile: memorizza il motivo.
                    // Il DISCONNECT arriverà subito dopo; lì verrà usato.
                    else if (pkt_type == PKT_VERSION_MISMATCH &&
                             ev.packet->dataLength >= sizeof(PktVersionMismatch)) {
                        PktVersionMismatch vm{};
                        std::memcpy(&vm, ev.packet->data, sizeof(vm));
                        printf("[client] PKT_VERSION_MISMATCH ricevuto: server=%u client=%u\n",
                               vm.server_version, PROTOCOL_VERSION);
                        pending_disc_reason = "Version Mismatch: please update your client.";
                        pending_disc_sub    = TextFormat(
                            "Server protocol: %u  \xe2\x80\x94  Your client: %u",
                            vm.server_version, PROTOCOL_VERSION);
                    }
                    // GameState broadcast: reconciliation + aggiornamento remoti.
                    else if (pkt_type == PKT_GAME_STATE &&
                             ev.packet->dataLength >= sizeof(PktGameState)) {
                        PktGameState resp{};
                        std::memcpy(&resp, ev.packet->data, sizeof(PktGameState));
                        last_game_state = resp.state;

                        // Aggiorna trail remote: solo quando arriva un tick
                        // effettivamente nuovo per quel player (evita clear spurii
                        // causati da broadcast generati da altri peer).
                        for (uint32_t i = 0; i < resp.state.count; i++) {
                            const PlayerState& rp = resp.state.players[i];
                            if (rp.player_id == 0 || rp.player_id == local_player_id) continue;
                            uint32_t& prev = remote_last_ticks[rp.player_id];
                            if (rp.last_processed_tick != prev) {
                                prev = rp.last_processed_tick;
                                TrailState& rt = remote_trails[rp.player_id];
                                if (rp.dash_active_ticks > 0) rt.Push(rp.x, rp.y);
                                else                          rt.Clear();
                            }
                        }

                        if (local_player_id != 0) {
                            for (uint32_t i = 0; i < resp.state.count; i++) {
                                const PlayerState& auth = resp.state.players[i];
                                if (auth.player_id != local_player_id) continue;

                                const uint32_t srv_tick = auth.last_processed_tick;
                                if (sim_tick > srv_tick &&
                                    sim_tick - srv_tick < IHIST) {
                                    // Reset allo stato autoritativo + replay
                                    // degli input in sospeso [srv_tick+1, sim_tick).
                                    player.SetState(auth);
                                    for (uint32_t t = srv_tick + 1; t < sim_tick; t++) {
                                        const InputFrame& hf = input_history[t % IHIST];
                                        if (hf.tick == t)
                                            player.Simulate(hf, world);
                                    }
                                } else if (sim_tick <= srv_tick) {
                                    // Server è avanti (non dovrebbe succedere).
                                    player.SetState(auth);
                                }
                                break;
                            }
                        }
                    }
                    // Carica il livello successivo o termina la partita.
                    else if (pkt_type == PKT_LOAD_LEVEL &&
                             ev.packet->dataLength >= sizeof(PktLoadLevel)) {
                        PktLoadLevel lpkt{};
                        std::memcpy(&lpkt, ev.packet->data, sizeof(lpkt));
                        if (lpkt.is_last) {
                            session_error       = "Game over.";
                            session_error_sub   = "Returning to main menu...";
                            session_error_color = {80, 220, 80, 255};
                            session_over = true;
                        } else {
                            world.LoadFromFile(lpkt.path);
                            PlayerState new_ps{};
                            new_ps.player_id = local_player_id;
                            std::strncpy(new_ps.name, menu.username, sizeof(new_ps.name) - 1);
                            for (int ty2 = 0; ty2 < world.GetHeight(); ty2++)
                                for (int tx2 = 0; tx2 < world.GetWidth(); tx2++)
                                    if (world.GetTile(tx2, ty2) == 'X') {
                                        new_ps.x = static_cast<float>(tx2 * TILE_SIZE);
                                        new_ps.y = static_cast<float>(ty2 * TILE_SIZE);
                                    }
                            player.SetState(new_ps);
                            prev_x = new_ps.x;  prev_y = new_ps.y;
                            camera.target = {new_ps.x + TILE_SIZE * 0.5f,
                                             new_ps.y + TILE_SIZE * 0.5f};
                            sim_tick      = 0;
                            accumulator   = 0.f;
                            jump_pressed  = dash_pending = restart_pending = false;
                            prev_finished = false;
                            show_record   = false;
                            best_ticks    = 0;
                            trail.Clear();
                            remote_trails.clear();
                            remote_last_ticks.clear();
                            last_game_state = {};
                            std::memset(input_history, 0, sizeof(input_history));
                        }
                    }
                    enet_packet_destroy(ev.packet);
                } else if (ev.type == ENET_EVENT_TYPE_DISCONNECT) {
                    if (!pending_disc_reason.empty()) {
                        // Motivo noto: arrivato via PKT_VERSION_MISMATCH prima del DISCONNECT.
                        session_error     = pending_disc_reason;
                        session_error_sub = pending_disc_sub;
                    } else if (ev.data == DISCONNECT_SERVER_BUSY) {
                        session_error     = "Server is busy. Try again later.";
                        session_error_sub = "A game is already in progress on this server.";
                    } else if (ev.data == DISCONNECT_VERSION_MISMATCH) {
                        // Fallback: motivo codificato nel campo data del DISCONNECT.
                        session_error = "Version Mismatch: please update your client.";
                    } else if (session_error.empty()) {
                        session_error = "Disconnected from server.";
                    }
                    session_over = true;
                }
                if (session_over) break;
            }
        }

        // --- Camera smooth follow + interpolazione visiva ---
        // Renderizza lo stato predetto localmente: nessun lag visivo.
        // server_state è mantenuto aggiornato per la futura reconciliation (passo 16).
        const PlayerState& local = player.GetState();

        // Rilevamento completamento livello e aggiornamento record.
        if (local.finished && !prev_finished) {
            if (best_ticks == 0 || local.level_ticks < best_ticks) {
                best_ticks  = local.level_ticks;
                show_record = true;
            }
        }
        prev_finished = local.finished;
        const float alpha  = accumulator / FIXED_DT;
        const float draw_x = prev_x + (local.x - prev_x) * alpha;
        const float draw_y = prev_y + (local.y - prev_y) * alpha;

        const float cx = draw_x + TILE_SIZE * 0.5f;
        const float cy = draw_y + TILE_SIZE * 0.5f;
        const float k  = 1.f - expf(-CAM_SMOOTH * dt);
        camera.target.x += (cx - camera.target.x) * k;
        camera.target.y += (cy - camera.target.y) * k;
        // Aggiorna l'offset ogni frame: mantiene il player centrato anche dopo resize.
        camera.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};

        // --- Render ---
        BeginDrawing();
        ClearBackground({8, 12, 40, 255});

        BeginMode2D(camera);
            DrawTilemap(world);
            // Trail remoti (aggiornati sui dati autoritativi).
            for (uint32_t i = 0; i < last_game_state.count; i++) {
                const PlayerState& rp = last_game_state.players[i];
                if (rp.player_id == 0 || rp.player_id == local_player_id) continue;
                remote_trails[rp.player_id].Draw({255, 140, 60, 200});
            }
            trail.Draw({0, 220, 255, 255});
            // Altri giocatori (posizione autoritativa dal server, non interpolata).
            if (local_player_id != 0) {
                for (uint32_t i = 0; i < last_game_state.count; i++) {
                    const PlayerState& rp = last_game_state.players[i];
                    if (rp.player_id != 0 && rp.player_id != local_player_id)
                        DrawPlayer(font_hud, rp.x, rp.y, rp, false);
                }
            }
            DrawPlayer(font_hud, draw_x, draw_y, local);
        EndMode2D();

        DrawHUD(font_hud, local, last_game_state.count, menu.choice != MenuChoice::OFFLINE);
#ifndef NDEBUG
        DrawDebugPanel(font_small, local);
#endif

        // --- Timer al centro in alto (nascosto in lobby) ---
        if (!last_game_state.is_lobby) {
            {
                const uint32_t total_cs = local.level_ticks * 100 / 60;
                const uint32_t mins =  total_cs / 6000;
                const uint32_t secs = (total_cs % 6000) / 100;
                const uint32_t cs   =  total_cs % 100;
                const char* t_str = TextFormat("%02u:%02u.%02u", mins, secs, cs);
                const Color   t_col = local.finished ? Color{0, 230, 100, 255} : WHITE;
                const Vector2 t_sz  = MeasureTextEx(font_timer, t_str, 48, 1);
                DrawTextEx(font_timer, t_str,
                    {GetScreenWidth() * 0.5f - t_sz.x * 0.5f, 10}, 48, 1, t_col);
            }

            // --- Best time (sotto il timer, centro in alto) ---
            if (best_ticks > 0) {
                const uint32_t b_cs   = best_ticks * 100 / 60;
                const uint32_t b_mins =  b_cs / 6000;
                const uint32_t b_secs = (b_cs % 6000) / 100;
                const uint32_t b_frac =  b_cs % 100;
                const char* b_str = TextFormat("Best: %02u:%02u.%02u", b_mins, b_secs, b_frac);
                const Vector2 b_sz = MeasureTextEx(font_hud, b_str, 24, 1);
                DrawTextEx(font_hud, b_str,
                    {GetScreenWidth() * 0.5f - b_sz.x * 0.5f, 62}, 24, 1, {0, 220, 255, 200});
            }
        }

        // --- Tempo limite livello (top right, aggiorna 1 volta/s) ---
        if (last_game_state.time_limit_secs > 0) {
            const uint32_t tl_m = last_game_state.time_limit_secs / 60;
            const uint32_t tl_s = last_game_state.time_limit_secs % 60;
            const char* tl_str = TextFormat("%02u:%02u", tl_m, tl_s);
            const Vector2 tl_sz = MeasureTextEx(font_hud, tl_str, 24, 1);
            const Color tl_col = last_game_state.time_limit_secs <= 30
                ? Color{255, 80, 80, 255}   // rosso negli ultimi 30 s
                : Color{255, 255, 255, 200};
            DrawTextEx(font_hud, tl_str,
                {GetScreenWidth() - tl_sz.x - 10, 10}, 24, 1, tl_col);
        }

        // --- "New Record!" (nascosto in lobby) ---
        if (show_record && !last_game_state.is_lobby) {
            const char* msg = "New Record!";
            const Vector2 ms = MeasureTextEx(font_hud, msg, 24, 1);
            // Sfondo semitrasparente per leggibilità.
            DrawRectangle(
                static_cast<int>(GetScreenWidth()  * 0.5f - ms.x * 0.5f - 12),
                static_cast<int>(GetScreenHeight() * 0.5f - 20),
                static_cast<int>(ms.x + 24), 44,
                {0, 0, 0, 160});
            DrawTextEx(font_hud, msg,
                {GetScreenWidth() * 0.5f - ms.x * 0.5f,
                 GetScreenHeight() * 0.5f - 12.f},
                24, 1, {0, 220, 255, 255});
        }

        // --- Lobby: hint + grande conto alla rovescia centralizzato ---
        if (last_game_state.is_lobby) {
            const uint32_t cd = last_game_state.next_level_countdown_ticks;
            const float cx    = GetScreenWidth()  * 0.5f;
            const float cy    = GetScreenHeight() * 0.5f;
            if (cd > 0) {
                // Conto alla rovescia: 3... 2... 1...
                const uint32_t rem_secs = (cd + 59u) / 60u;  // ceil: 180→3, 120→2, 60→1
                const char* cd_str = TextFormat("Game starts in %u...", rem_secs);
                const Vector2 cd_sz = MeasureTextEx(font_timer, cd_str, 48, 1);
                DrawRectangle(0, static_cast<int>(cy - 40),
                              GetScreenWidth(), 80, {0, 0, 0, 160});
                DrawTextEx(font_timer, cd_str,
                    {cx - cd_sz.x * 0.5f, cy - 24.f},
                    48, 1, {0, 220, 255, 255});
            } else {
                // Suggerimento: porta tutti i giocatori all'uscita.
                const char* hint = last_game_state.count == 0
                    ? "Waiting for players..."
                    : "Move ALL players to the EXIT to start!";
                const Vector2 hs = MeasureTextEx(font_hud, hint, 24, 1);
                DrawTextEx(font_hud, hint,
                    {cx - hs.x * 0.5f, GetScreenHeight() - hs.y - 20.f},
                    24, 1, {200, 200, 220, 220});
            }
        // --- "Next level in: X.XX s" (bottom right, solo durante il gioco) ---
        } else if (last_game_state.next_level_countdown_ticks > 0) {
            const uint32_t rem_cs   = last_game_state.next_level_countdown_ticks * 100 / 60;
            const uint32_t rem_secs = rem_cs / 100;
            const uint32_t rem_frac = rem_cs % 100;
            const char* cd_str = TextFormat("Next level in: %u.%02u s", rem_secs, rem_frac);
            const Vector2 cd_sz = MeasureTextEx(font_hud, cd_str, 24, 1);
            DrawTextEx(font_hud, cd_str,
                {GetScreenWidth()  - cd_sz.x - 10,
                 GetScreenHeight() - cd_sz.y - 10},
                24, 1, {0, 220, 255, 200});
        }

        // --- Menu pausa ---
        if (pause_state != PauseState::PLAYING) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{5, 5, 15, 200});
            const float pcx = GetScreenWidth()  * 0.5f;
            const float pcy = GetScreenHeight() * 0.5f;
            static constexpr Color PAU_ACCENT = {  0, 220, 255, 255};
            static constexpr Color PAU_TXT    = {200, 200, 220, 255};
            static constexpr Color PAU_DIM    = { 80,  80, 100, 255};

            if (pause_state == PauseState::PAUSED) {
                const char* title = "PAUSED";
                const Vector2 ts = MeasureTextEx(font_timer, title, 48, 1);
                DrawTextEx(font_timer, title, {pcx - ts.x * 0.5f, pcy - 110.f}, 48, 1, PAU_ACCENT);

                const char* items[2] = {"Resume", "Quit to Menu"};
                for (int i = 0; i < 2; i++) {
                    const bool  sel = (pause_focused == i);
                    const Color col = sel ? PAU_ACCENT : PAU_TXT;
                    const char* buf = TextFormat("%s%s", sel ? "> " : "  ", items[i]);
                    const Vector2 sz = MeasureTextEx(font_hud, buf, 24, 1);
                    DrawTextEx(font_hud, buf, {pcx - sz.x * 0.5f, pcy - 10.f + i * 44.f}, 24, 1, col);
                }

                const char* hint = "up/down: navigate   enter: confirm   esc: resume";
                const Vector2 hs = MeasureTextEx(font_small, hint, 18, 1);
                DrawTextEx(font_small, hint, {pcx - hs.x * 0.5f, pcy + 120.f}, 18, 1, PAU_DIM);

            } else { // CONFIRM_QUIT
                const char* msg = "Quit to main menu?";
                const Vector2 ms = MeasureTextEx(font_hud, msg, 24, 1);
                DrawTextEx(font_hud, msg, {pcx - ms.x * 0.5f, pcy - 90.f}, 24, 1, PAU_TXT);

                const char* items[2] = {"No", "Yes"};
                for (int i = 0; i < 2; i++) {
                    const bool  sel = (confirm_focused == i);
                    const Color col = sel ? PAU_ACCENT : PAU_TXT;
                    const char* buf = TextFormat("%s%s", sel ? "> " : "  ", items[i]);
                    const Vector2 sz = MeasureTextEx(font_hud, buf, 24, 1);
                    DrawTextEx(font_hud, buf, {pcx - sz.x * 0.5f, pcy - 10.f + i * 44.f}, 24, 1, col);
                }

                const char* hint = "up/down: navigate   enter: confirm   esc: back";
                const Vector2 hs = MeasureTextEx(font_small, hint, 18, 1);
                DrawTextEx(font_small, hint, {pcx - hs.x * 0.5f, pcy + 100.f}, 18, 1, PAU_DIM);
            }
        }

        EndDrawing();
    }

    // Disconnessione ENet pulita (net_peer può essere nullptr se già disconnesso).
    if (net_peer && net_peer->state == ENET_PEER_STATE_CONNECTED)
        enet_peer_disconnect_now(net_peer, 0);
    enet_host_destroy(net_host);

    // Ferma il server locale (no-op se non avviato).
    local_srv.Stop();

    // Mostra errore di sessione per 2 secondi prima di tornare al menu.
    if (!session_error.empty()) {
        const float cx  = GetScreenWidth()  * 0.5f;
        const float cy  = GetScreenHeight() * 0.5f;
        const double t0 = GetTime();
        while (!WindowShouldClose() && GetTime() - t0 < 3.0) {
            BeginDrawing();
            ClearBackground({8, 12, 40, 255});
            // Titolo principale
            const Vector2 ms = MeasureTextEx(font_hud, session_error.c_str(), 24, 1);
            DrawTextEx(font_hud, session_error.c_str(),
                {cx - ms.x * 0.5f, cy - 22.f},
                24, 1, session_error_color);
            // Dettaglio versione
            if (!session_error_sub.empty()) {
                const Vector2 ms2 = MeasureTextEx(font_small, session_error_sub.c_str(), 18, 1);
                DrawTextEx(font_small, session_error_sub.c_str(),
                    {cx - ms2.x * 0.5f, cy + 8.f},
                    18, 1, {180, 180, 180, 255});
            }
            EndDrawing();
        }
    }

    } // fine outer loop menu --> partita

    enet_deinitialize();

    UnloadFont(font_small);
    UnloadFont(font_hud);
    UnloadFont(font_timer);
    CloseWindow();
    return 0;
}