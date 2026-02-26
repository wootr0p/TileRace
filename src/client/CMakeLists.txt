# --- CLIENT: Visuals & Network Sync ---
add_executable(TileRace_Client main.cpp)

# Include necessari: Rete + la cartella src di Raylib (se non esportata)
target_include_directories(TileRace_Client PRIVATE 
    "${enet_SOURCE_DIR}/include"
)

# Dipendenze: Il "Triangolo" completo
target_link_libraries(TileRace_Client PRIVATE 
    common_logic 
    raylib 
    enet
)

# Dipendenze di sistema Windows per Grafica e Rete
if(WIN32)
    target_compile_definitions(TileRace_Client PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        NOGDI
        NOUSER
    )

    target_link_libraries(TileRace_Client PRIVATE 
        ws2_32   # ENet
        winmm    # ENet / Raylib Timer
        gdi32    # Raylib Graphics
        user32   # Raylib Window management
        shell32  # Raylib System calls
    )
endif()

# Macro per distinguere il codice client (es. per la prediction)
target_compile_definitions(TileRace_Client PRIVATE CLIENT_BUILD)