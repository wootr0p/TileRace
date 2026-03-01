#pragma once
#include <string>
#include <vector>

// World contiene la tilemap caricata da file .txt.
// Zero dipendenze esterne: solo STL.
//
// Caratteri riconosciuti:
//   '0' = muro        (solido)
//   'E' = endpoint    (non solido — vincere toccandolo; il player ci passa attraverso)
//   'X' = spawn       (non solido — posizione iniziale giocatori)
//   ' ' = aria        (non solido)
class World {
public:
    // Carica la mappa da file. Ritorna true se il file è stato aperto con successo.
    bool LoadFromFile(const char* path);

    // Ritorna true se il tile (tx, ty) è solido ('0' o 'E').
    // Fuori dai limiti --> false (non si rimane incastrati ai bordi della mappa).
    bool IsSolid(int tx, int ty) const;

    // Ritorna il carattere grezzo del tile, o ' ' se fuori dai limiti.
    char GetTile(int tx, int ty) const;

    // Dimensioni della mappa in tile.
    int GetWidth()  const { return width_; }
    int GetHeight() const { return height_; }

private:
    std::vector<std::string> rows_;
    int width_  = 0;
    int height_ = 0;
};
