# --- COMMON: Pure Logic & Data Structures ---
add_library(common_logic STATIC 
    Protocol.h
    GameLogic.cpp 
)

# PUBLIC rende disponibile la cartella common a chiunque linki questa lib
target_include_directories(common_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Nessun linkaggio esterno qui. Solo C++ Standard.