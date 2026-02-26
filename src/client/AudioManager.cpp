#include "AudioManager.h"
#include <raylib.h>
#include <iostream>

bool AudioManager::Init(const std::string& assets_path) {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        std::cerr << "[Audio] ATTENZIONE: dispositivo audio non disponibile.\n";
        return false;
    }
    initialized_ = true;
    // TODO: caricare i file audio
    // sounds_[AUDIO_JUMP] = LoadSound((assets_path + "/sfx/jump.wav").c_str());
    (void)assets_path;
    return true;
}

void AudioManager::Shutdown() {
    if (!initialized_) return;
    // TODO: UnloadSound per ogni suono caricato
    CloseAudioDevice();
    initialized_ = false;
}

void AudioManager::PlayEvent(const PktAudioEvent& event) {
    if (!initialized_) return;

    // TODO: PlaySound(sounds_[event.event_id]);
    // Per ora, log di debug
    std::cout << "[Audio] Evento " << static_cast<int>(event.event_id)
              << " per giocatore " << event.player_id << "\n";
}

void AudioManager::Update() {
    // Per ora vuoto. Utile in futuro per stream musicali o suoni loopati.
}
