// UIWidgets.cpp — implementazione degli helper di disegno UI.

#include "UIWidgets.h"
#include "Colors.h"
#include <cstring>
#include <cstdio>

namespace UIWidgets {

static constexpr float BTN_SZ = 24.f;
static constexpr float BTN_SP = 10.f;

bool Hovered(Rectangle r) {
    return CheckCollisionPointRec(GetMousePosition(), r);
}

bool Clicked(Rectangle r) {
    return Hovered(r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

Rectangle DrawTextBtn(Font& f, const char* label, float cx, float y, bool selected) {
    const Vector2 ts = MeasureTextEx(f, label, BTN_SZ, 1);
    Rectangle r = {cx - ts.x * 0.5f - 20, y, ts.x + 40, BTN_SZ + BTN_SP};
    const Color col = selected ? CLRS_ACCENT : CLRS_TEXT_MAIN;
    const char* prefix = selected ? "> " : "  ";
    char buf[64];
    snprintf(buf, sizeof(buf), "%s%s", prefix, label);
    const Vector2 bts = MeasureTextEx(f, buf, BTN_SZ, 1);
    DrawTextEx(f, buf, {cx - bts.x * 0.5f, y}, BTN_SZ, 1, col);
    return r;
}

void DrawTextField(Font& f, const char* label,
                   float cx, float y, const char* buf, bool focused) {
    // Label
    const Vector2 lts = MeasureTextEx(f, label, BTN_SZ, 1);
    DrawTextEx(f, label, {cx - lts.x * 0.5f, y}, BTN_SZ, 1, CLRS_TEXT_DIM);
    y += BTN_SZ + 4;

    // Valore
    const char* display = buf[0] ? buf : "_";
    const Color vcol    = focused ? CLRS_ACCENT : CLRS_TEXT_MAIN;
    const Vector2 vts   = MeasureTextEx(f, display, BTN_SZ, 1);
    DrawTextEx(f, display, {cx - vts.x * 0.5f, y}, BTN_SZ, 1, vcol);

    // Cursore lampeggiante se attivo
    if (focused && ((int)(GetTime() * 2) % 2 == 0) && buf[0]) {
        DrawTextEx(f, "_", {cx - vts.x * 0.5f + vts.x, y}, BTN_SZ, 1, CLRS_ACCENT);
    }

    // Linea sotto il valore
    const float line_w = 260.f;
    const Color line_c = focused ? CLRS_ACCENT : CLRS_TEXT_DIM;
    DrawLineEx({cx - line_w * 0.5f, y + BTN_SZ + 2}, {cx + line_w * 0.5f, y + BTN_SZ + 2},
               focused ? 1.5f : 1.f, line_c);
}

bool PollFieldInput(char* buf, int maxlen) {
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

void DrawCentered(Font& f, const char* text, float cx, float y, float sz, Color col) {
    const Vector2 ts = MeasureTextEx(f, text, sz, 1);
    DrawTextEx(f, text, {cx - ts.x * 0.5f, y}, sz, 1, col);
}

} // namespace UIWidgets
