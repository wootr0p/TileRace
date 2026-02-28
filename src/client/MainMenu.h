#pragma once
#include <raylib.h>
#include <cstdint>

enum class MenuChoice { OFFLINE, ONLINE, QUIT };

struct MenuResult {
    MenuChoice choice          = MenuChoice::OFFLINE;
    char       server_ip[64]   = "127.0.0.1";
    char       username[16]    = "Player";
    uint16_t   port            = 7777;
};

// Mostra il menu principale (blocca finché l'utente non sceglie).
// La finestra deve già essere aperta (InitWindow già chiamato).
MenuResult ShowMainMenu(Font& font);
