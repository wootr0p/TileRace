# --- SERVER ---
# File aggiunti passo per passo:
#   passo 11 --> main.cpp + TileRace_Server (ping/pong ENet)  ← COMPLETATO
#   passo 14 --> server autoritativo multi-giocatore          ← COMPLETATO
#   passo 20 --> ServerLogic.cpp --> server_logic lib (condiviso con client per offline) ← COMPLETATO

# --- server_logic: logica del server riusabile (standalone + LocalServer) ---
add_library(server_logic STATIC ServerLogic.cpp)
target_include_directories(server_logic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}      # ServerLogic.h accessibile a chi linka
    ${enet_SOURCE_DIR}/include
)
target_link_libraries(server_logic PUBLIC common_logic enet)
if(WIN32)
    target_compile_definitions(server_logic PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(server_logic PUBLIC ws2_32 winmm)
endif()
target_compile_features(server_logic PRIVATE cxx_std_20)

# --- TileRace_Server: entry point standalone ---
add_executable(TileRace_Server main.cpp)
target_link_libraries(TileRace_Server PRIVATE server_logic)
if(WIN32)
    target_compile_definitions(TileRace_Server PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()
target_compile_features(TileRace_Server PRIVATE cxx_std_20)
