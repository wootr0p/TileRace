#pragma once
#include <raylib.h>
#include <cstdint>
#include "SaveData.h"

enum class MenuChoice { OFFLINE, ONLINE, QUIT };

struct MenuResult {
    MenuChoice choice        = MenuChoice::OFFLINE;
    char       server_ip[64] = "127.0.0.1";
    char       username[16]  = "Player";
    uint16_t   port          = 7777;
};

// Blocking splash screen — shows title + "Press any key or gamepad button to start".
// The first input device that triggers dismissal is "claimed" for the lifetime of the window:
//   - Keyboard press  → returns -1  (keyboard-only mode; gamepads ignored thereafter)
//   - Gamepad press   → returns the gamepad index 0-based (gamepad-only mode; keyboard
//                        input is ignored in-game for movement/action bindings)
// The window must already be open (InitWindow already called).
int ShowSplashScreen(Font& font);

// Blocking menu loop — returns only when the user confirms a choice.
// The window must already be open (InitWindow already called).
// Reads initial field values from `save` and writes them back before returning.
// gamepad_index: the gamepad claimed at splash screen time (-1 = keyboard only).
MenuResult ShowMainMenu(Font& font, SaveData& save, int gamepad_index);
