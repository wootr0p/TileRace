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

// Blocking splash screen — shows title + "Press any button to start".
// Returns when any key, mouse button, or gamepad button is pressed.
// Returns the index of the gamepad that was pressed (0-based), or -1 if a
// keyboard/mouse input triggered the dismiss (no gamepad claimed).
// The window must already be open (InitWindow already called).
int ShowSplashScreen(Font& font);

// Blocking menu loop — returns only when the user confirms a choice.
// The window must already be open (InitWindow already called).
// Reads initial field values from `save` and writes them back before returning.
// gamepad_index: the gamepad claimed at splash screen time (-1 = keyboard only).
MenuResult ShowMainMenu(Font& font, SaveData& save, int gamepad_index);
