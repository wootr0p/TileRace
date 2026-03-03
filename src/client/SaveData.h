#pragma once
#include <cstdint>

// Persistent player data. Serialised as JSON then XOR-obfuscated before writing to disk.
// Callers work only with this struct; Load/Save handle all I/O.
struct SaveData {
    char username[16] = {};
    char last_ip[64]  = {};
    bool sfx_muted    = false;
};

// Load from "save.dat" in the CWD. Returns true if the file was valid.
// On error (missing file, bad checksum) the struct is left zero-initialised.
bool LoadSaveData(SaveData& out);

void SaveSaveData(const SaveData& data);
