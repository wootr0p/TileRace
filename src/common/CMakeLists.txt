# --- COMMON: Pure Logic & Data Structures ---
# REGOLA: nessun source file qui può includere Raylib, ENet o qualsiasi
# libreria grafica/di rete. Solo C++ Standard Library.
#
# File aggiunti passo per passo:
#   passo 3  → World.h / World.cpp          ← COMPLETATO
#   passo 4  → Physics.h, PlayerState.h, Player.h / Player.cpp  ← COMPLETATO
#   passo 4  → PlayerState.h, Player.h / Player.cpp
#   passo 5  → Physics.h
#   passo 9  → InputFrame.h
#   passo 11 → Protocol.h
#   passo 14 → GameState.h  ← COMPLETATO
add_library(common_logic STATIC
    World.cpp
    Player.cpp
)
target_include_directories(common_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(common_logic PUBLIC cxx_std_20)