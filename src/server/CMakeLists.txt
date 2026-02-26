# --- SERVER_LOGIC: Logica server riusabile ----
# Separata dall'eseguibile così TileRace_Client può linkarla
# per la modalità offline (LocalServer).
add_library(server_logic STATIC GameServer.cpp)

target_include_directories(server_logic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${enet_SOURCE_DIR}/include"
)

target_link_libraries(server_logic PUBLIC common_logic enet)

if(WIN32)
    target_link_libraries(server_logic PUBLIC ws2_32 winmm)
endif()

target_compile_features(server_logic PUBLIC cxx_std_20)

# --- SERVER: Headless Authority ---
add_executable(TileRace_Server main.cpp)

target_include_directories(TileRace_Server PRIVATE
    "${enet_SOURCE_DIR}/include"
)

target_link_libraries(TileRace_Server PRIVATE server_logic)

target_compile_definitions(TileRace_Server PRIVATE SERVER_BUILD)