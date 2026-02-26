# --- CLIENT: Visuals & Network Sync ---
add_executable(TileRace_Client
    main.cpp
    GameClient.cpp
    InputHandler.cpp
    Renderer.cpp
    AudioManager.cpp
    MainMenu.cpp
    LocalServer.cpp
)

target_include_directories(TileRace_Client PRIVATE
    "${enet_SOURCE_DIR}/include"
)

target_link_libraries(TileRace_Client PRIVATE
    common_logic
    server_logic
    raylib
    enet
)

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

target_compile_definitions(TileRace_Client PRIVATE CLIENT_BUILD)
target_compile_features(TileRace_Client PRIVATE cxx_std_20)

# In Release non mostrare la console Windows (subsystem WINDOWS invece di CONSOLE)
if(WIN32)
    set_target_properties(TileRace_Client PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()