# --- COMMON: Pure Logic & Data Structures ---
# REGOLA: nessun source file qui può includere Raylib, ENet o qualsiasi
# libreria grafica/di rete. Solo C++ Standard Library.
add_library(common_logic STATIC
    Player.cpp
    World.cpp
)

# Gli header sono parte dell'interfaccia pubblica della libreria:
# chiunque linki common_logic può includere direttamente i .h di questa cartella
target_include_directories(common_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# C++20 per structured bindings, std::span, etc.
target_compile_features(common_logic PUBLIC cxx_std_20)