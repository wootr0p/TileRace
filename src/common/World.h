#pragma once
#include <vector>
#include <string>

// =============================================================================
// World.h — Rappresentazione della mappa (tilemap) e collision detection
// =============================================================================
//
// Il World è "cieco" rispetto al rendering. Sa solo quali tile sono solidi.
// Il Renderer (nel client) è responsabile di disegnarlo.
//
// ARCHITETTURA: World è in /common perché il SERVER deve poterlo usare per
// la collision detection autoritativa. Il client lo usa per la prediction.
// Entrambi caricano la STESSA mappa dallo STESSO file per garantire
// la consistenza. (In futuro: il server potrebbe inviare la mappa al client.)
// =============================================================================

class World {
public:
    // Carica la mappa da un file di testo ASCII
    // Formato: '.' = vuoto, '#' = solido
    bool LoadFromFile(const std::string& path);

    // Ritorna true se il tile alle coordinate di gioco (tile_x, tile_y) è solido
    [[nodiscard]] bool IsSolid(int tile_x, int tile_y) const;

    // Controlla se un rettangolo AABB (Axis-Aligned Bounding Box) collide con
    // la mappa e, se sì, calcola il vettore di correzione minimo.
    // pos_x, pos_y: angolo top-left del bounding box
    // w, h: dimensioni
    // out_x, out_y: posizione corretta (output)
    // Ritorna true se c'è stata almeno una collisione
    bool ResolveCollision(float pos_x, float pos_y, float w, float h,
                          float& out_x, float& out_y) const;

    [[nodiscard]] int GetWidth()  const { return width_;  }
    [[nodiscard]] int GetHeight() const { return height_; }

private:
    // tiles_[y][x] = 0: vuoto, 1: solido
    std::vector<std::vector<int>> tiles_;
    int width_  = 0;
    int height_ = 0;
};
