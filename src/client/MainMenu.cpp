#include "MainMenu.h"
#include <raylib.h>
#include <algorithm>

// ---- Palette (coerente con il Renderer) ----
static constexpr Color MN_BG       = {  5,   5,  15, 255 };
static constexpr Color MN_ACCENT   = {  0, 230, 200, 255 };  // ciano
static constexpr Color MN_DIM      = { 60, 140, 140, 255 };
static constexpr Color MN_WHITE    = {220, 220, 220, 255 };
static constexpr Color MN_HOVER    = {  0, 255, 180, 255 };  // ciano-verde
static constexpr Color MN_OFFLINE  = { 30, 180, 100, 255 };  // verde neon
static constexpr Color MN_ONLINE   = {  0, 160, 255, 255 };  // blu neon
static constexpr Color MN_ERROR    = {255,  60,  60, 255 };
static constexpr Color MN_INPUT_BG = { 15,  15,  35, 255 };
static constexpr Color MN_INPUT_BD = {  0, 200, 200, 255 };

// Helper: rettangolo con bordo neon + testo centrato
static void DrawButton(Rectangle r, const char* label, bool hovered, Color base) {
    Color fill   = hovered ? Color{base.r, base.g, base.b, 220}
                            : Color{static_cast<unsigned char>(base.r/3),
                                    static_cast<unsigned char>(base.g/3),
                                    static_cast<unsigned char>(base.b/3), 200};
    Color border = hovered ? base : MN_DIM;
    DrawRectangleRec(r, fill);
    DrawRectangleLinesEx(r, 2.0f, border);
    int font_size = 22;
    int tw = MeasureText(label, font_size);
    DrawText(label,
             static_cast<int>(r.x + r.width  / 2 - tw / 2),
             static_cast<int>(r.y + r.height / 2 - font_size / 2),
             font_size,
             hovered ? MN_WHITE : Color{200, 200, 200, 200});
}

// Helper: testo centrato orizzontalmente
static void DrawCentered(const char* text, int y, int font_size, Color color) {
    int w = GetScreenWidth();
    int tw = MeasureText(text, font_size);
    DrawText(text, (w - tw) / 2, y, font_size, color);
}

// =============================================================================

MenuResult MainMenu::Show() {
    MenuResult result{};
    State state = State::Main;
    std::string ip_buf = "localhost";
    bool invalid_ip = false;

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        bool clicked  = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

        int W = GetScreenWidth();
        int H = GetScreenHeight();
        int cx = W / 2;

        if (state == State::Main) {
            // ---- Layout pulsanti ----
            Rectangle r_offline = { static_cast<float>(cx - 160), H * 0.52f, 320, 64 };
            Rectangle r_online  = { static_cast<float>(cx - 160), H * 0.52f + 84, 320, 64 };

            bool h_off = CheckCollisionPointRec(mouse, r_offline);
            bool h_on  = CheckCollisionPointRec(mouse, r_online);

            // ---- Draw ----
            BeginDrawing();
            ClearBackground(MN_BG);
            DrawMain(h_off, h_on);
            EndDrawing();

            // ---- Input ----
            if (clicked) {
                if (h_off) {
                    result.choice    = MenuChoice::Offline;
                    result.server_ip = "localhost";
                    result.port      = 1234;
                    return result;
                }
                if (h_on) {
                    state      = State::OnlineInput;
                    invalid_ip = false;
                }
            }

        } else {
            // ---- Layout input online ----
            Rectangle r_input   = { static_cast<float>(cx - 200), H * 0.48f,       400, 48 };
            Rectangle r_connect = { static_cast<float>(cx - 160), H * 0.48f + 72,  320, 56 };
            Rectangle r_back    = { static_cast<float>(cx - 80),  H * 0.48f + 148, 160, 44 };

            bool h_conn = CheckCollisionPointRec(mouse, r_connect);
            bool h_back = CheckCollisionPointRec(mouse, r_back);

            // Testo input
            HandleTextInput(ip_buf);

            // ---- Draw ----
            BeginDrawing();
            ClearBackground(MN_BG);
            DrawOnlineInput(ip_buf, h_conn, h_back, invalid_ip);
            EndDrawing();

            // ---- Input ----
            if (clicked) {
                if (h_back) {
                    state = State::Main;
                }
                if (h_conn || IsKeyPressed(KEY_ENTER)) {
                    if (ip_buf.empty()) {
                        invalid_ip = true;
                    } else {
                        result.choice    = MenuChoice::Online;
                        result.server_ip = ip_buf;
                        result.port      = 1234;
                        return result;
                    }
                }
            }
            // INVIO diretto
            if (IsKeyPressed(KEY_ENTER) && !ip_buf.empty()) {
                result.choice    = MenuChoice::Online;
                result.server_ip = ip_buf;
                result.port      = 1234;
                return result;
            }
        }
    }

    result.choice = MenuChoice::Quit;
    return result;
}

// =============================================================================
// Draw helpers
// =============================================================================

void MainMenu::DrawMain(bool hover_offline, bool hover_online) const {
    int W = GetScreenWidth();
    int H = GetScreenHeight();
    int cx = W / 2;

    // Titolo
    DrawCentered("TILE", static_cast<int>(H * 0.15f), 80, MN_ACCENT);
    DrawCentered("RACE", static_cast<int>(H * 0.15f + 78), 80, MN_HOVER);

    // Sottotitolo
    DrawCentered("Platform Multiplayer", static_cast<int>(H * 0.40f), 20, MN_DIM);

    // Pulsanti
    Rectangle r_offline = { static_cast<float>(cx - 160), H * 0.52f, 320, 64 };
    Rectangle r_online  = { static_cast<float>(cx - 160), H * 0.52f + 84, 320, 64 };

    DrawButton(r_offline, "OFFLINE  (locale)", hover_offline, MN_OFFLINE);
    DrawButton(r_online,  "ONLINE   (rete)",   hover_online,  MN_ONLINE);

    // Footer
    DrawCentered("ESC / chiudi finestra per uscire",
                 static_cast<int>(H * 0.92f), 16, MN_DIM);
}

void MainMenu::DrawOnlineInput(const std::string& ip_buf, bool hover_connect,
                                bool hover_back, bool invalid) const {
    int W = GetScreenWidth();
    int H = GetScreenHeight();
    int cx = W / 2;

    DrawCentered("CONNETTI AL SERVER", static_cast<int>(H * 0.28f), 32, MN_ACCENT);
    DrawCentered("Inserisci l'indirizzo IP del server:", static_cast<int>(H * 0.40f), 18, MN_WHITE);

    // Campo testo
    Rectangle r_input = { static_cast<float>(cx - 200), H * 0.48f, 400, 48 };
    DrawRectangleRec(r_input, MN_INPUT_BG);
    DrawRectangleLinesEx(r_input, 2.0f, invalid ? MN_ERROR : MN_INPUT_BD);
    // Testo + cursore lampeggiante
    std::string display = ip_buf + ((static_cast<int>(GetTime() * 2) % 2 == 0) ? "|" : " ");
    DrawText(display.c_str(),
             static_cast<int>(r_input.x) + 10,
             static_cast<int>(r_input.y + r_input.height / 2 - 11),
             22, MN_ACCENT);

    if (invalid) {
        DrawCentered("Indirizzo non valido", static_cast<int>(H * 0.48f + 54), 16, MN_ERROR);
    }

    Rectangle r_connect = { static_cast<float>(cx - 160), H * 0.48f + 72,  320, 56 };
    Rectangle r_back    = { static_cast<float>(cx - 80),  H * 0.48f + 148, 160, 44 };

    DrawButton(r_connect, "CONNETTI", hover_connect, MN_ONLINE);
    DrawButton(r_back,    "INDIETRO", hover_back,    MN_DIM);

    DrawCentered("INVIO per connettere", static_cast<int>(H * 0.48f + 210), 16, MN_DIM);
}

bool MainMenu::HandleTextInput(std::string& buf) const {
    // Backspace
    if (IsKeyPressed(KEY_BACKSPACE) && !buf.empty()) {
        buf.pop_back();
    }

    // Caratteri ASCII validi per un IP (0-9, a-z, A-Z, '.', '-')
    int ch;
    while ((ch = GetCharPressed()) != 0) {
        if (buf.size() < 64 &&
            ((ch >= '0' && ch <= '9') ||
             (ch >= 'a' && ch <= 'z') ||
             (ch >= 'A' && ch <= 'Z') ||
              ch == '.' || ch == '-' || ch == '_')) {
            buf += static_cast<char>(ch);
        }
    }

    return IsKeyPressed(KEY_ENTER);
}
