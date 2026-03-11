// MainMenu.cpp — menu iniziale minimalista (passo 19).
// Due schermate: MAIN (Offline/Online + nome), ONLINE (IP + connetti).
// Tutto testo, SpaceMono-Regular, nessun riquadro.

#include "MainMenu.h"
#include "Colors.h"
#include "UIWidgets.h"
#include "SaveData.h"
#include "Protocol.h"
#include <cstring>
#include <cstdio>
#include <cmath>

static constexpr float SZ = 24.f;   // dimensione unica del font

// ---------------------------------------------------------------------------
// ShowSplashScreen
// ---------------------------------------------------------------------------
int ShowSplashScreen(Font& font) {
    Font font_title = LoadFontEx("assets/fonts/SpaceMono-Regular.ttf", 96, nullptr, 0);

    // Salta gli input nel primo frame per evitare falsi positivi
    bool skip_input = true;
    double blink_timer = 0.0;
    int claimed_gp = -2;  // -2 = niente premuto; -1 = tastiera reclamata; >=0 = indice gamepad

    while (!WindowShouldClose()) {
        const float W  = (float)GetScreenWidth();
        const float H  = (float)GetScreenHeight();
        const float cx = W * 0.5f;

        // La prima pressione determina il dispositivo per l'intera durata della finestra:
        //   - Qualsiasi tasto sulla tastiera → modalità tastiera (gamepad ignorato)
        //   - Qualsiasi tasto su un gamepad  → modalità gamepad (tastiera ignorata in gioco)
        if (!skip_input) {
            // Controlla prima i gamepad
            for (int i = 0; i < 4; ++i) {
                if (!IsGamepadAvailable(i)) continue;
                for (int b = 0; b <= GAMEPAD_BUTTON_RIGHT_THUMB; ++b) {
                    if (IsGamepadButtonPressed(i, (GamepadButton)b)) {
                        claimed_gp = i;
                        break;
                    }
                }
                if (claimed_gp >= 0) break;
            }
            // Poi la tastiera (solo se nessun gamepad ha già risposto)
            if (claimed_gp < 0 && GetKeyPressed() != 0) {
                claimed_gp = -1;  // modalità tastiera
            }
        }

        blink_timer += GetFrameTime();

        // Controlla se almeno un gamepad è collegato (per il messaggio)
        bool any_gp = false;
        for (int i = 0; i < 4; ++i) if (IsGamepadAvailable(i)) { any_gp = true; break; }

        const bool blink_visible = fmod(blink_timer, 1.0) < 0.65;  // 650ms on, 350ms off

        BeginDrawing();
        skip_input = false;
        ClearBackground(CLRS_BG_MENU);

        // Titolo grande centrato a circa 1/3 dall'alto
        UIWidgets::DrawCentered(font_title, "Tile Race", cx, H * 0.32f, 96.f, CLRS_ACCENT);

        // Messaggio principale con blink
        if (blink_visible) {
            if (any_gp)
                UIWidgets::DrawCentered(font, "Press any key or gamepad button to start",
                                        cx, H * 0.60f, SZ, CLRS_TEXT_MAIN);
            else
                UIWidgets::DrawCentered(font, "Press any key to start",
                                        cx, H * 0.60f, SZ, CLRS_TEXT_MAIN);
        }

        // Sottotitolo fisso: spiega la semantica della scelta del dispositivo
        UIWidgets::DrawCentered(font,
            "The first input device you use will be exclusively assigned to this window",
            cx, H * 0.60f + SZ + 10.f, SZ * 0.75f, CLRS_TEXT_DIM);

        // Versione angolo in basso a destra
        {
            const char* ver = TextFormat("v%s", GAME_VERSION);
            const Vector2 vs = MeasureTextEx(font, ver, SZ * 0.6f, 1);
            DrawTextEx(font, ver, {W - vs.x - 10, H - vs.y - 8}, SZ * 0.6f, 1, CLRS_TEXT_DIM);
        }

        EndDrawing();

        if (claimed_gp != -2) break;  // -1 = tastiera, >=0 = gamepad
    }

    UnloadFont(font_title);
    return claimed_gp;  // -1 = modalità tastiera, >=0 = indice gamepad
}

// ---------------------------------------------------------------------------
// ShowMainMenu
// ---------------------------------------------------------------------------
MenuResult ShowMainMenu(Font& font, SaveData& save, int gamepad_index) {
    // Font grande per i titoli: caricato qui perché LoadFontEx
    // richiede InitWindow già chiamato, e la dimensione deve coincidere
    // con quella di render per evitare il pixelato da upscale.
    Font font_title = LoadFontEx("assets/fonts/SpaceMono-Regular.ttf", 64, nullptr, 0);
    MenuResult res{};

    // Pre-riempi dai dati salvati.
    if (save.username[0] != '\0')
        std::strncpy(res.username, save.username, sizeof(res.username) - 1);
    if (save.last_ip[0] != '\0')
        std::strncpy(res.server_ip, save.last_ip, sizeof(res.server_ip) - 1);

    enum class Screen { MAIN, ONLINE } screen = Screen::MAIN;
    // btn_focus: 0 = pulsante sinistro, 1 = pulsante destro (per entrambe le schermate)
    int   btn_focus = 0;
    // Edge detection stick sinistro (valori del frame precedente)
    float prev_sx = 0.f, prev_sy = 0.f;
    // Salta gli input nel primo frame: BeginDrawing() non è ancora stato chiamato quindi
    // PollInputEvents() non ha ancora azzerato i tasti rimasti premuti dalla sessione di gioco.
    bool skip_input = true;

    while (!WindowShouldClose()) {
        const float W  = (float)GetScreenWidth();
        const float H  = (float)GetScreenHeight();
        const float cx = W * 0.5f;

        // ---- Gamepad + stick edge ----
        const bool gp = (gamepad_index >= 0) && IsGamepadAvailable(gamepad_index);
        const float sx = gp ? GetGamepadAxisMovement(gamepad_index, GAMEPAD_AXIS_LEFT_X) : 0.f;
        const float sy = gp ? GetGamepadAxisMovement(gamepad_index, GAMEPAD_AXIS_LEFT_Y) : 0.f;
        const bool stick_right = (prev_sx <  0.5f && sx >=  0.5f);
        const bool stick_left  = (prev_sx > -0.5f && sx <= -0.5f);
        const bool stick_down  = (prev_sy <  0.5f && sy >=  0.5f);
        const bool stick_up    = (prev_sy > -0.5f && sy <= -0.5f);
        prev_sx = sx; prev_sy = sy;

        // ---- Navigazione bottoni (frecce / D-pad / stick)
        // WASD esclusi: D/S andrebbero in conflitto con la digitazione nel campo testo.
        const bool nav_next = !skip_input && (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_DOWN) ||
                              (gp && (IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ||
                                      IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_LEFT_FACE_DOWN))) ||
                              stick_right || stick_down);
        const bool nav_prev = !skip_input && (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_UP) ||
                              (gp && (IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_LEFT_FACE_LEFT) ||
                                      IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_LEFT_FACE_UP))) ||
                              stick_left || stick_up);
        // Conferma: Enter o Cross (no Space: aggiungerebbe un carattere al campo testo)
        const bool nav_ok = !skip_input && (IsKeyPressed(KEY_ENTER) ||
                            (gp && IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)));

        if (nav_next) btn_focus = 1;
        if (nav_prev) btn_focus = 0;

        // ---- Input testo nel campo attivo ----
        if (screen == Screen::MAIN) UIWidgets::PollFieldInput(res.username,  (int)sizeof(res.username));
        else                        UIWidgets::PollFieldInput(res.server_ip, (int)sizeof(res.server_ip));

        // ---- Azioni ----
        MenuChoice pending_choice = MenuChoice::QUIT;
        bool       has_action     = false;

        if (screen == Screen::ONLINE) {
            if (!skip_input && IsKeyPressed(KEY_ESCAPE)) {
                screen    = Screen::MAIN;
                btn_focus = 0;
            }
            if (nav_ok && !has_action) {
                if (btn_focus == 1) { pending_choice = MenuChoice::ONLINE; has_action = true; }
                else                { screen = Screen::MAIN; btn_focus = 0; }
            }
        } else { // Screen::MAIN
            // ESC o Start gamepad --> esci dal gioco
            if (!skip_input &&
                (IsKeyPressed(KEY_ESCAPE) ||
                (gp && IsGamepadButtonPressed(gamepad_index, GAMEPAD_BUTTON_MIDDLE_RIGHT)))) {
                pending_choice = MenuChoice::QUIT;
                has_action     = true;
            }
            if (nav_ok && !has_action) {
                if (btn_focus == 0) { pending_choice = MenuChoice::OFFLINE; has_action = true; }
                else                { screen = Screen::ONLINE; btn_focus = 0; }
            }
        }

        // ---- Render ----
        BeginDrawing();
        skip_input = false;  // da ora PollInputEvents() ha azzerato i pressed del frame precedente
        ClearBackground(CLRS_BG_MENU);

        if (screen == Screen::MAIN) {
            // Titolo
            UIWidgets::DrawCentered(font_title, "Tile Race", cx, H * 0.18f, 64.f, CLRS_ACCENT);

            // Campo nome (sempre attivo per la digitazione)
            const float field_y = H * 0.40f;
            UIWidgets::DrawTextField(font, "name", cx, field_y, res.username, true);

            // Pulsanti
            const float btn_y = H * 0.62f;
            Rectangle r_off = UIWidgets::DrawTextBtn(font, "OFFLINE", cx - 120, btn_y, btn_focus == 0);
            Rectangle r_on  = UIWidgets::DrawTextBtn(font, "ONLINE",  cx + 120, btn_y, btn_focus == 1);

            // Aggiorna btn_focus se il mouse si sposta sopra un bottone
            if (UIWidgets::Hovered(r_off)) btn_focus = 0;
            if (UIWidgets::Hovered(r_on))  btn_focus = 1;

            if (UIWidgets::Clicked(r_off)) { pending_choice = MenuChoice::OFFLINE; has_action = true; }
            if (UIWidgets::Clicked(r_on))  { screen = Screen::ONLINE; btn_focus = 0; }

            // Hint
            UIWidgets::DrawCentered(font, "left/right: select   enter/cross: confirm   esc: quit",
                         cx, H - SZ - 16, SZ, CLRS_TEXT_DIM);

        } else {  // Screen::ONLINE
            // Titolo
            UIWidgets::DrawCentered(font_title, "ONLINE", cx, H * 0.18f, 64.f, CLRS_ACCENT);

            // Campo IP (sempre attivo per la digitazione)
            const float ip_y = H * 0.45f;
            UIWidgets::DrawTextField(font, "server ip", cx, ip_y, res.server_ip, true);

            // Pulsanti: 0=BACK (sinistra), 1=CONNECT (destra)
            const float btn_y = H * 0.66f;
            Rectangle r_back = UIWidgets::DrawTextBtn(font, "BACK",    cx - 110, btn_y, btn_focus == 0);
            Rectangle r_conn = UIWidgets::DrawTextBtn(font, "CONNECT", cx + 110, btn_y, btn_focus == 1);

            // Aggiorna btn_focus se il mouse si sposta sopra un bottone
            if (UIWidgets::Hovered(r_back)) btn_focus = 0;
            if (UIWidgets::Hovered(r_conn)) btn_focus = 1;

            if (UIWidgets::Clicked(r_conn)) { pending_choice = MenuChoice::ONLINE; has_action = true; }
            if (UIWidgets::Clicked(r_back)) { screen = Screen::MAIN; btn_focus = 0; }

            // Hint
            UIWidgets::DrawCentered(font, "left/right: select   enter/cross: confirm   esc: back",
                         cx, H - SZ - 16, SZ, CLRS_TEXT_DIM);
        }

        // Versione (angolo in basso a destra, visibile in entrambe le schermate)
        {
            const char* ver = TextFormat("v%s", GAME_VERSION);
            const Vector2 vs = MeasureTextEx(font, ver, SZ * 0.6f, 1);
            DrawTextEx(font, ver, {W - vs.x - 10, H - vs.y - 8}, SZ * 0.6f, 1, CLRS_TEXT_DIM);
        }

        EndDrawing();

        // Esci dopo EndDrawing
        if (has_action) {
            res.choice = pending_choice;
            if (res.username[0]  == '\0') std::strncpy(res.username,  "Player",    sizeof(res.username));
            if (res.server_ip[0] == '\0') std::strncpy(res.server_ip, "127.0.0.1", sizeof(res.server_ip));
            // Aggiorna e salva i dati persistenti.
            std::strncpy(save.username, res.username,  sizeof(save.username) - 1);
            std::strncpy(save.last_ip,  res.server_ip, sizeof(save.last_ip)  - 1);
            SaveSaveData(save);
            UnloadFont(font_title);
            return res;
        }
    }

    UnloadFont(font_title);
    res.choice = MenuChoice::QUIT;
    return res;
}


