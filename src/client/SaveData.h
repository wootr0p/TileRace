#pragma once
#include <cstdint>

// ---------------------------------------------------------------------------
// SaveData — dati persistenti del giocatore.
// Serializzati come JSON, poi offuscati con XOR prima della scrittura su disco.
// Le funzioni Load/Save gestiscono tutto; il chiamante lavora solo con la struct.
// ---------------------------------------------------------------------------

struct SaveData {
    char     username[16] = {};     // ultimo nickname usato
    char     last_ip[64]  = {};     // ultimo IP usato in ONLINE
    // futuro: uint32_t best_ticks[MAX_LEVELS] = {};
};

// Carica da "save.dat" nella CWD. Ritorna true se il file era valido.
// In caso di errore (file mancante, checksum errato) la struct rimane a zero.
bool LoadSaveData(SaveData& out);

// Salva su "save.dat" nella CWD.
void SaveSaveData(const SaveData& data);
