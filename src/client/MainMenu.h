#pragma once
#include <string>
#include <cstdint>

// =============================================================================
// MainMenu.h — Schermata iniziale: scelta Online / Offline
// =============================================================================
//
// MainMenu presuppone che la finestra Raylib sia già aperta.
// Blocca nel proprio loop finché l'utente non fa una scelta valida
// o chiude la finestra.
//
// SEPARAZIONE: MainMenu non sa niente di GameClient o GameServer.
// Restituisce solo dati (MenuResult) e lascia che main.cpp decida cosa fare.
// =============================================================================

enum class MenuChoice {
    Offline,   // Gioca in locale con server embedded
    Online,    // Connettiti a un server remoto (IP:porta)
    Quit,      // L'utente ha chiuso la finestra
};

struct MenuResult {
    MenuChoice  choice     = MenuChoice::Quit;
    std::string server_ip  = "localhost";
    uint16_t    port       = 1234;
};

class MainMenu {
public:
    // Mostra il menu e blocca finché l'utente non sceglie.
    // Presuppone finestra Raylib già aperta.
    [[nodiscard]] MenuResult Show();

private:
    enum class State { Main, OnlineInput };

    void DrawMain(bool hover_offline, bool hover_online) const;
    void DrawOnlineInput(const std::string& ip_buf, bool hover_connect,
                         bool hover_back, bool invalid) const;

    // Gestisce l'inserimento testo: aggiunge caratteri, backspace, invio
    // Ritorna true se l'utente ha premuto INVIO
    bool HandleTextInput(std::string& buf) const;
};
