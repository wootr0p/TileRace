// MainMenu.cpp — menu iniziale minimalista (passo 19).
// Due schermate: MAIN (Offline/Online + nome), ONLINE (IP + connetti).
// Tutto testo, SpaceMono-Regular, nessun riquadro.

#include "MainMenu.h"
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

// Disegna un pulsante testo con prefisso '> ' se hovered.
static Rectangle DrawTextBtn(Font& f, const char* label, float cx, float y) {
    const Vector2 ts = MeasureTextEx(f, label, SZ, 1);
    // area cliccabile: un po' più larga del testo
    Rectangle r = {cx - ts.x * 0.5f - 20, y, ts.x + 40, SZ + SP};
    const bool hv = Hovered(r);
    const Color col = hv ? ACCENT_COL : TXT_MAIN;
    // prefisso
    const char* prefix = hv ? "> " : "  ";
    char buf[64];
    snprintf(buf, sizeof(buf), "%s%s", prefix, label);
    const Vector2 bts = MeasureTextEx(f, buf, SZ, 1);
    DrawTextEx(f, buf, {cx - bts.x * 0.5f, y}, SZ, 1, col);
    return r;  // ritorna l'area per collision detection
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
MenuResult ShowMainMenu(Font& font) {
    // Font grande per i titoli: caricato qui perché LoadFontEx
    // richiede InitWindow già chiamato, e la dimensione deve coincidere
    // con quella di render per evitare il pixelato da upscale.
    Font font_title = LoadFontEx("assets/fonts/SpaceMono-Regular.ttf", 72, nullptr, 0);
    MenuResult res{};

    enum class Screen { MAIN, ONLINE } screen = Screen::MAIN;
    // 0 = campo nome, 1 = campo IP
    int focused = 0;

    while (!WindowShouldClose()) {
        const float W  = (float)GetScreenWidth();
        const float H  = (float)GetScreenHeight();
        const float cx = W * 0.5f;

        // ---- Input: focus con click ----
        // (gestito nel render pass perché le rect dei campi dipendono dal layout)

        // Tab per cambiare campo (solo nella schermata principale)
        if (IsKeyPressed(KEY_TAB) && screen == Screen::MAIN)
            focused = 0;  // un solo campo nome nella schermata principale

        // Input testo nel campo attivo
        if (screen == Screen::MAIN) PollFieldInput(res.username,  (int)sizeof(res.username));
        else                        PollFieldInput(res.server_ip, (int)sizeof(res.server_ip));

        // ---- Azioni tasti ----
        MenuChoice pending_choice = MenuChoice::QUIT;
        bool       has_action     = false;

        if (screen == Screen::ONLINE) {
            if (IsKeyPressed(KEY_ENTER)) {
                pending_choice = MenuChoice::ONLINE;
                has_action     = true;
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                screen  = Screen::MAIN;
                focused = 0;
            }
        }

        // ---- Render ----
        BeginDrawing();
        ClearBackground(BG_COL);

        if (screen == Screen::MAIN) {
            // Titolo
            DrawCentered(font_title, "TILE RACE", cx, H * 0.18f, 64.f, ACCENT_COL);

            // Campo nome
            const float field_y = H * 0.40f;
            // Area click campo nome
            Rectangle r_name = {cx - 150, field_y - 4, 300, SZ * 2 + 16};
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                CheckCollisionPointRec(GetMousePosition(), r_name))
                focused = 0;
            DrawTextField(font, "name", cx, field_y, res.username, focused == 0);

            // Pulsanti
            const float btn_y = H * 0.62f;
            Rectangle r_off = DrawTextBtn(font, "OFFLINE", cx - 120, btn_y);
            Rectangle r_on  = DrawTextBtn(font, "ONLINE",  cx + 120, btn_y);

            if (Clicked(r_off)) { pending_choice = MenuChoice::OFFLINE; has_action = true; }
            if (Clicked(r_on))  { screen = Screen::ONLINE; focused = 0; }

            // Hint
            DrawCentered(font, "tab: switch field", cx, H - SZ - 16, SZ * 0.6f, TXT_DIM);

        } else {  // Screen::ONLINE
            // Titolo
            DrawCentered(font_title, "ONLINE", cx, H * 0.18f, 48.f, ACCENT_COL);

            // Campo IP — unico campo
            const float ip_y = H * 0.45f;
            DrawTextField(font, "server ip", cx, ip_y, res.server_ip, true);

            // Pulsanti
            const float btn_y = H * 0.66f;
            Rectangle r_conn = DrawTextBtn(font, "CONNECT", cx - 110, btn_y);
            Rectangle r_back = DrawTextBtn(font, "BACK",    cx + 110, btn_y);

            if (Clicked(r_conn)) { pending_choice = MenuChoice::ONLINE; has_action = true; }
            if (Clicked(r_back)) { screen = Screen::MAIN; focused = 0; }

            // Hint
            DrawCentered(font, "enter: connect   esc: back",
                         cx, H - SZ - 16, SZ * 0.6f, TXT_DIM);
        }

        EndDrawing();

        // Esci dopo EndDrawing
        if (has_action) {
            res.choice = pending_choice;
            if (res.username[0]  == '\0') std::strncpy(res.username,  "Player",    sizeof(res.username));
            if (res.server_ip[0] == '\0') std::strncpy(res.server_ip, "127.0.0.1", sizeof(res.server_ip));
            UnloadFont(font_title);
            return res;
        }
    }

    UnloadFont(font_title);
    res.choice = MenuChoice::QUIT;
    return res;
}


