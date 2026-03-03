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
// The window must already be open (InitWindow already called).
void ShowSplashScreen(Font& font);

// Blocking menu loop — returns only when the user confirms a choice.
// The window must already be open (InitWindow already called).
// Reads initial field values from `save` and writes them back before returning.
MenuResult ShowMainMenu(Font& font, SaveData& save);
