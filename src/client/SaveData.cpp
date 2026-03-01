// SaveData.cpp — serializzazione offuscata dei dati di gioco.
//
// Schema file "save.dat":
//   [4 bytes] magic     = 0x54524356  ("TRCV")
//   [4 bytes] payload_len
//   [4 bytes] checksum  = somma dei byte del JSON grezzo (mod 2^32)
//   [N bytes] payload   = JSON XOR'd con la chiave ciclica
//
// Il JSON grezzo ha il formato:
//   {"u":"<username>","i":"<last_ip>"}
// Campi futuri (es. record) possono essere aggiunti senza rompere
// la retrocompatibilità se si usa il parser incrementale già presente.

#include "SaveData.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>

// ---------------------------------------------------------------------------
// Chiave XOR (16 byte)
// ---------------------------------------------------------------------------
static constexpr uint8_t XOR_KEY[]  = {
    0x4B, 0x9F, 0x2A, 0xD3, 0x61, 0xB8, 0x07, 0xE5,
    0x3C, 0x74, 0xAF, 0x19, 0x52, 0xCC, 0x86, 0x3E
};
static constexpr size_t  KEY_LEN    = sizeof(XOR_KEY);
static constexpr uint32_t MAGIC     = 0x54524356u;  // "TRCV"
static constexpr const char* SAVE_PATH = "save.dat";

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string XorBuffer(const std::string& src) {
    std::string out(src.size(), '\0');
    for (size_t i = 0; i < src.size(); i++)
        out[i] = static_cast<char>(
            static_cast<uint8_t>(src[i]) ^ XOR_KEY[i % KEY_LEN]);
    return out;
}

static uint32_t Checksum(const std::string& s) {
    uint32_t sum = 0;
    for (unsigned char c : s) sum += c;
    return sum;
}

// Escape basilare: sostituisce " con \" e \ con \\ nel valore.
static std::string JsonEscape(const char* s) {
    std::string out;
    for (const char* p = s; *p; ++p) {
        if (*p == '\\') out += "\\\\";
        else if (*p == '"') out += "\\\"";
        else out += *p;
    }
    return out;
}

// Legge il valore stringa dopo la chiave JSON "key" nel buffer raw.
// Riempie out (max outLen-1 chars). Ritorna true se trovata.
static bool JsonReadStr(const std::string& json,
                        const char* key, char* out, size_t outLen) {
    // Cerca "key":
    std::string needle = std::string("\"") + key + "\":\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return false;
    pos += needle.size();
    std::string val;
    bool escape = false;
    while (pos < json.size()) {
        char c = json[pos++];
        if (escape) { val += c; escape = false; }
        else if (c == '\\') escape = true;
        else if (c == '"') break;
        else val += c;
    }
    std::strncpy(out, val.c_str(), outLen - 1);
    out[outLen - 1] = '\0';
    return true;
}

// ---------------------------------------------------------------------------
// API pubblica
// ---------------------------------------------------------------------------
bool LoadSaveData(SaveData& out) {
    out = SaveData{};
    FILE* f = std::fopen(SAVE_PATH, "rb");
    if (!f) return false;

    uint32_t magic = 0, len = 0, csum = 0;
    if (std::fread(&magic, 4, 1, f) != 1 || magic != MAGIC) { std::fclose(f); return false; }
    if (std::fread(&len,   4, 1, f) != 1 || len > 4096)     { std::fclose(f); return false; }
    if (std::fread(&csum,  4, 1, f) != 1)                    { std::fclose(f); return false; }

    std::string payload(len, '\0');
    if (std::fread(payload.data(), 1, len, f) != len) { std::fclose(f); return false; }
    std::fclose(f);

    // Deoffusca
    const std::string json = XorBuffer(payload);

    // Verifica checksum
    if (Checksum(json) != csum) return false;

    JsonReadStr(json, "u", out.username, sizeof(out.username));
    JsonReadStr(json, "i", out.last_ip,  sizeof(out.last_ip));
    return true;
}

void SaveSaveData(const SaveData& data) {
    // Serializza in JSON
    const std::string json =
        std::string("{\"u\":\"") + JsonEscape(data.username) +
        "\",\"i\":\"" + JsonEscape(data.last_ip) + "\"}";

    const uint32_t csum    = Checksum(json);
    const std::string enc  = XorBuffer(json);
    const uint32_t len     = static_cast<uint32_t>(enc.size());

    FILE* f = std::fopen(SAVE_PATH, "wb");
    if (!f) return;
    std::fwrite(&MAGIC, 4, 1, f);
    std::fwrite(&len,   4, 1, f);
    std::fwrite(&csum,  4, 1, f);
    std::fwrite(enc.data(), 1, len, f);
    std::fclose(f);
}
