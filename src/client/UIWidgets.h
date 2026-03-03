#pragma once
// Stateless Raylib UI helpers — text rendering, buttons, text fields.
// No internal state. Depends only on Raylib and Colors.h.
#include <raylib.h>

namespace UIWidgets {

bool Hovered(Rectangle r);
bool Clicked(Rectangle r);

// Centred text button. Returns its Rectangle for hit-testing.
// selected=true adds "> " prefix and uses CLRS_ACCENT.
Rectangle DrawTextBtn(Font& f, const char* label, float cx, float y, bool selected = false);

// Text input field with label, value, and blinking cursor.
void DrawTextField(Font& f, const char* label,
                   float cx, float y, const char* buf, bool focused);

// Handle keyboard input for a text field (backspace + GetCharPressed).
// Returns true if the buffer was modified.
bool PollFieldInput(char* buf, int maxlen);

// Text centred horizontally on cx.
void DrawCentered(Font& f, const char* text, float cx, float y, float sz, Color col);

} // namespace UIWidgets
