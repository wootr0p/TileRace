// MainMenu.cpp — menu iniziale minimalista (passo 19).
// Due schermate: MAIN (Offline/Online + nome), ONLINE (IP + connetti).
// Tutto testo, SpaceMono-Regular, nessun riquadro.

#include "MainMenu.h"
#include "SaveData.h"
#include "Protocol.h"
#include <cstring>
#include <cstdio>

// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------
static constexpr Color BG_COL     = {  5,   5,  15, 255};
static constexpr Color ACCENT_COL = {  0, 220, 255, 255};
static constexpr Color TXT_MAIN   = {200, 200, 220, 255};
static constexpr Color TXT_DIM    = { 80,  80, 100, 255};

static constexpr float SZ = 24.f;   // dimensione unica del font
static constexpr float SP = 10.f;   // spaziatura verticale tra righe

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static bool Hovered(Rectangle r) {
    return CheckCollisionPointRec(GetMousePosition(), r);
}
static bool Clicked(Rectangle r) {
    return Hovered(r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Disegna un pulsante testo con prefisso '> ' se selezionato.
static Rectangle DrawTextBtn(Font& f, const char* label, float cx, float y, bool selected = false) {
    const Vector2 ts = MeasureTextEx(f, label, SZ, 1);
    Rectangle r = {cx - ts.x * 0.5f - 20, y, ts.x + 40, SZ + SP};
    const Color col = selected ? ACCENT_COL : TXT_MAIN;
    const char* prefix = selected ? "> " : "  ";
    char buf[64];
    snprintf(buf, sizeof(buf), "%s%s", prefix, label);
    const Vector2 bts = MeasureTextEx(f, buf, SZ, 1);
    DrawTextEx(f, buf, {cx - bts.x * 0.5f, y}, SZ, 1, col);
    return r;
}

// Disegna un campo testo con label e linea di separazione.
static void DrawTextField(Font& f, const char* label,
                          float cx, float y, const char* buf, bool focused) {
    // Label
    const Vector2 lts = MeasureTextEx(f, label, SZ, 1);
    DrawTextEx(f, label, {cx - lts.x * 0.5f, y}, SZ, 1, TXT_DIM);
    y += SZ + 4;

    // Valore
    const char* display = buf[0] ? buf : "_";
    const Color vcol    = focused ? ACCENT_COL : TXT_MAIN;
    const Vector2 vts   = MeasureTextEx(f, display, SZ, 1);
    DrawTextEx(f, display, {cx - vts.x * 0.5f, y}, SZ, 1, vcol);

    // Cursore lampeggiante se attivo
    if (focused && ((int)(GetTime() * 2) % 2 == 0) && buf[0]) {
        DrawTextEx(f, "_", {cx - vts.x * 0.5f + vts.x, y}, SZ, 1, ACCENT_COL);
    }

    // Linea sotto il valore
    const float line_w = 260.f;
    const Color line_c = focused ? ACCENT_COL : TXT_DIM;
    DrawLineEx({cx - line_w * 0.5f, y + SZ + 2}, {cx + line_w * 0.5f, y + SZ + 2},
               focused ? 1.5f : 1.f, line_c);
}

// Gestisce l'input da tastiera su un campo testo.
static bool PollFieldInput(char* buf, int maxlen) {
    bool changed = false;
    if (IsKeyPressed(KEY_BACKSPACE)) {
        const int len = (int)std::strlen(buf);
        if (len > 0) { buf[len - 1] = '\0'; changed = true; }
    }
    int ch;
    while ((ch = GetCharPressed()) != 0) {
        const int len = (int)std::strlen(buf);
        if (len < maxlen - 1) {
            buf[len]     = (char)ch;
            buf[len + 1] = '\0';
            changed = true;
        }
    }
    return changed;
}

// Testo centrato.
static void DrawCentered(Font& f, const char* text, float cx, float y,
                         float sz, Color col) {
    const Vector2 ts = MeasureTextEx(f, text, sz, 1);
    DrawTextEx(f, text, {cx - ts.x * 0.5f, y}, sz, 1, col);
}

// ---------------------------------------------------------------------------
// ShowMainMenu
// ---------------------------------------------------------------------------
MenuResult ShowMainMenu(Font& font, SaveData& save) {
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
        const bool gp = IsGamepadAvailable(0);
        const float sx = gp ? GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) : 0.f;
        const float sy = gp ? GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y) : 0.f;
        const bool stick_right = (prev_sx <  0.5f && sx >=  0.5f);
        const bool stick_left  = (prev_sx > -0.5f && sx <= -0.5f);
        const bool stick_down  = (prev_sy <  0.5f && sy >=  0.5f);
        const bool stick_up    = (prev_sy > -0.5f && sy <= -0.5f);
        prev_sx = sx; prev_sy = sy;

        // ---- Navigazione bottoni (frecce / D-pad / stick)
        // WASD esclusi: D/S andrebbero in conflitto con la digitazione nel campo testo.
        const bool nav_next = !skip_input && (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_DOWN) ||
                              (gp && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ||
                                      IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN))) ||
                              stick_right || stick_down);
        const bool nav_prev = !skip_input && (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_UP) ||
                              (gp && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) ||
                                      IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP))) ||
                              stick_left || stick_up);
        // Conferma: Enter o Cross (no Space: aggiungerebbe un carattere al campo testo)
        const bool nav_ok = !skip_input && (IsKeyPressed(KEY_ENTER) ||
                            (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)));

        if (nav_next) btn_focus = 1;
        if (nav_prev) btn_focus = 0;

        // ---- Input testo nel campo attivo ----
        if (screen == Screen::MAIN) PollFieldInput(res.username,  (int)sizeof(res.username));
        else                        PollFieldInput(res.server_ip, (int)sizeof(res.server_ip));

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
                (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)))) {
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
        ClearBackground(BG_COL);

        if (screen == Screen::MAIN) {
            // Titolo
            DrawCentered(font_title, "Tile Race", cx, H * 0.18f, 64.f, ACCENT_COL);

            // Campo nome (sempre attivo per la digitazione)
            const float field_y = H * 0.40f;
            DrawTextField(font, "name", cx, field_y, res.username, true);

            // Pulsanti
            const float btn_y = H * 0.62f;
            Rectangle r_off = DrawTextBtn(font, "OFFLINE", cx - 120, btn_y, btn_focus == 0);
            Rectangle r_on  = DrawTextBtn(font, "ONLINE",  cx + 120, btn_y, btn_focus == 1);

            // Aggiorna btn_focus se il mouse si sposta sopra un bottone
            if (Hovered(r_off)) btn_focus = 0;
            if (Hovered(r_on))  btn_focus = 1;

            if (Clicked(r_off)) { pending_choice = MenuChoice::OFFLINE; has_action = true; }
            if (Clicked(r_on))  { screen = Screen::ONLINE; btn_focus = 0; }

            // Hint
            DrawCentered(font, "left/right: select   enter/cross: confirm   esc: quit",
                         cx, H - SZ - 16, SZ, TXT_DIM);

        } else {  // Screen::ONLINE
            // Titolo
            DrawCentered(font_title, "ONLINE", cx, H * 0.18f, 64.f, ACCENT_COL);

            // Campo IP (sempre attivo per la digitazione)
            const float ip_y = H * 0.45f;
            DrawTextField(font, "server ip", cx, ip_y, res.server_ip, true);

            // Pulsanti: 0=BACK (sinistra), 1=CONNECT (destra)
            const float btn_y = H * 0.66f;
            Rectangle r_back = DrawTextBtn(font, "BACK",    cx - 110, btn_y, btn_focus == 0);
            Rectangle r_conn = DrawTextBtn(font, "CONNECT", cx + 110, btn_y, btn_focus == 1);

            // Aggiorna btn_focus se il mouse si sposta sopra un bottone
            if (Hovered(r_back)) btn_focus = 0;
            if (Hovered(r_conn)) btn_focus = 1;

            if (Clicked(r_conn)) { pending_choice = MenuChoice::ONLINE; has_action = true; }
            if (Clicked(r_back)) { screen = Screen::MAIN; btn_focus = 0; }

            // Hint
            DrawCentered(font, "left/right: select   enter/cross: confirm   esc: back",
                         cx, H - SZ - 16, SZ, TXT_DIM);
        }

        // Versione (angolo in basso a destra, visibile in entrambe le schermate)
        {
            const char* ver = TextFormat("v%s", GAME_VERSION);
            const Vector2 vs = MeasureTextEx(font, ver, SZ * 0.6f, 1);
            DrawTextEx(font, ver, {W - vs.x - 10, H - vs.y - 8}, SZ * 0.6f, 1, TXT_DIM);
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


