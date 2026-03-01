# --- CLIENT: Visuals & Network Sync ---
# I file verranno aggiunti passo per passo:
#   passo 2  → main.cpp (tilemap da file, tutto inline) ← COMPLETATO
#   passo 3  → World da common (nessun .cpp nuovo qui)
#   passo 9  → InputHandler.h / InputHandler.cpp
#   passo 13 → GameClient.h / GameClient.cpp
#   passo 2+ → Renderer.h / Renderer.cpp
#   passo 19 → MainMenu.h / MainMenu.cpp                ← COMPLETATO
#   passo 20 → LocalServer.h / LocalServer.cpp          ← COMPLETATO
#   fase avanzata → AudioManager.h / AudioManager.cpp
add_executable(TileRace
    main.cpp
    MainMenu.cpp
    LocalServer.cpp
    SaveData.cpp
    WinIcon.cpp
)

# ENet non esporta include dir come PUBLIC, quindi la aggiungiamo manualmente.
# Aggiungiamo anche src/server per ServerLogic.h (usato da LocalServer).
target_include_directories(TileRace PRIVATE
    ${enet_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../server
)

target_link_libraries(TileRace PRIVATE
    common_logic
    server_logic
    raylib
    enet
)

if(WIN32)
    target_compile_definitions(TileRace PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        NOGDI
        NOUSER
    )
    target_link_libraries(TileRace PRIVATE
        winmm    # Raylib Timer
        gdi32    # Raylib Graphics
        user32   # Raylib Window management
        shell32  # Raylib System calls
        ws2_32   # ENet networking
    )
endif()

target_compile_definitions(TileRace PRIVATE CLIENT_BUILD)
target_compile_features(TileRace PRIVATE cxx_std_20)

# In Release non mostrare la console Windows (subsystem WINDOWS invece di CONSOLE)
if(WIN32)
    set_target_properties(TileRace PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()
