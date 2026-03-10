// main.cpp — punto di ingresso di TileRace.
// Responsabilità: init finestra, loop esterno menu → sessione → menu.
// Tutta la logica di gioco vive in GameSession.
// Tutta la logica di rete vive in NetworkClient.

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <raylib.h>
#include <enet/enet.h>
#include "Protocol.h"
#include "GameMode.h"
#include "Renderer.h"
#include "MainMenu.h"
#include "LocalServer.h"
#include "SaveData.h"
#include "WinIcon.h"
#include "NetworkClient.h"
#include "GameSession.h"

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    // Parse optional --gamepad <N> argument (default: 0).
    // Allows two client instances on the same machine to use different gamepads.
    int gamepad_index = 0;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--gamepad") == 0) {
            if (i + 1 < argc) {
                gamepad_index = std::atoi(argv[i + 1]);
                if (gamepad_index < 0) gamepad_index = 0;
            }
            break;
        }
    }
    if (enet_initialize() != 0) {
        fprintf(stderr, "[client] ERRORE: enet_initialize fallita\n");
        return 1;
    }
    printf("[client] TileRace v%s  (protocol %u)\n", GAME_VERSION, PROTOCOL_VERSION);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, TextFormat("TileRace v%s", GAME_VERSION));
    InitAudioDevice();
    // Imposta l'icona sulla finestra Win32 nativa: carica dalla risorsa embedded
    // (ID 1 nel .rc), poi la forza sia come ICON_SMALL che ICON_BIG in modo che
    // la taskbar e alt+tab la mostrino correttamente anche dopo che GLFW ha
    // inizializzato la finestra con la sua icona di default.
    ApplyExeIconToWindow(GetWindowHandle());
    SetTargetFPS(120);
    SetExitKey(KEY_NULL);  // ESC gestito manualmente (pausa in-game, quit da menu)

    Renderer renderer;
    renderer.Init();

    // Carica i dati salvati (nickname, ultimo IP).
    SaveData save{};
    LoadSaveData(save);

    // -----------------------------------------------------------------------
    // Schermata iniziale (splash)
    // -----------------------------------------------------------------------
    ShowSplashScreen(renderer.HudFont(), gamepad_index);

    // -----------------------------------------------------------------------
    // Loop principale: menu --> partita --> menu (riparte se la connessione fallisce)
    // -----------------------------------------------------------------------
    while (!WindowShouldClose()) {

    // Menu iniziale (passo 19)
    const MenuResult menu = ShowMainMenu(renderer.HudFont(), save, gamepad_index);
    if (menu.choice == MenuChoice::QUIT) break;

    // In modalità offline il server genera direttamente il primo livello,
    // senza passare dalla lobby (skip_lobby = true).
    // Default game mode for offline is RACE.
    LocalServer local_srv;
    const bool is_offline = (menu.choice == MenuChoice::OFFLINE);
    const char* initial_map = LOBBY_MAP_PATH;
    if (is_offline)
        local_srv.Start(SERVER_PORT_LOCAL, initial_map, /*skip_lobby=*/true, GameMode::RACE);

    const char*    connect_ip   = is_offline ? "127.0.0.1"      : menu.server_ip;
    const uint16_t connect_port = is_offline ? SERVER_PORT_LOCAL : SERVER_PORT;

    NetworkClient net;
    if (!net.Connect(connect_ip, connect_port)) {
        local_srv.Stop();
        const double t0 = GetTime();
        while (!WindowShouldClose() && GetTime() - t0 < 2.0)
            renderer.DrawConnectionErrorScreen("Connection failed. Check IP and port.");
        continue;
    }

    const GameSession::Config cfg{ is_offline ? nullptr : initial_map, menu.username, is_offline, &save, gamepad_index };
    GameSession session(cfg);
    while (!WindowShouldClose() && !session.IsOver())
        session.Tick(GetFrameTime(), net, renderer);

    net.Disconnect();
    local_srv.Stop();

    if (!session.GetEndMessage().empty()) {
        const double t0 = GetTime();
        while (!WindowShouldClose() && GetTime() - t0 < 3.0)
            renderer.DrawSessionEndScreen(session.GetEndMessage().c_str(),
                                          session.GetEndSubMsg().c_str(),
                                          session.GetEndColor());
    }

    } // fine loop esterno


    enet_deinitialize();

    renderer.Cleanup();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
