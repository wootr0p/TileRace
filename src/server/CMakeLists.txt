# --- SERVER: Headless Authority ---
add_executable(TileRace_Server main.cpp)

# Include necessari: la nostra logica e la rete scaricata da FetchContent
target_include_directories(TileRace_Server PRIVATE 
    "${enet_SOURCE_DIR}/include"
)

# Dipendenze: Common (per le regole) e ENet (per i pacchetti)
target_link_libraries(TileRace_Server PRIVATE 
    common_logic 
    enet
)

# Networking su Windows richiede le API di sistema
if(WIN32)
    target_link_libraries(TileRace_Server PRIVATE ws2_32 winmm)
endif()

# Definiamo un macro per compilare codice condizionale se necessario
target_compile_definitions(TileRace_Server PRIVATE SERVER_BUILD)