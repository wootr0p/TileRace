set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
cmake_minimum_required(VERSION 3.16)
project(TileRace LANGUAGES C CXX)

# --- CONFIGURAZIONE GLOBALE ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(FetchContent)

# --- 1. RAYLIB ---
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
)

# --- 2. ENET ---
# Usiamo il mirror ufficiale su GitHub per ENet
FetchContent_Declare(
  enet
  GIT_REPOSITORY https://github.com/lsalzman/enet.git
  GIT_TAG master
)

# Scarica e rendi disponibili entrambe
FetchContent_MakeAvailable(raylib enet)

# --- CONFIGURAZIONE ASSETS ---
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" 
     DESTINATION "${CMAKE_BINARY_DIR}/bin")

# --- MODULI DEL PROGETTO ---
add_subdirectory(src/common)
add_subdirectory(src/client)
add_subdirectory(src/server)

# --- INSTALLAZIONE RUNTIME (solo ciò che serve per distribuire il gioco) ---
set(TILERACE_RUNTIME_COMPONENT TileRaceRuntime)

install(TARGETS TileRace_Client TileRace_Server
  RUNTIME DESTINATION .
  COMPONENT ${TILERACE_RUNTIME_COMPONENT}
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
  DESTINATION .
  COMPONENT ${TILERACE_RUNTIME_COMPONENT}
)

# --- LOGICA DI RUN (Identica alla tua precedente) ---
if(WIN32)
    add_custom_target(run
        COMMAND start cmd /c "$<TARGET_FILE:TileRace_Server>"
        COMMAND ${CMAKE_COMMAND} -E sleep 1
        COMMAND "$<TARGET_FILE:TileRace_Client>"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        DEPENDS TileRace_Server TileRace_Client
        COMMENT "Avvio del Server e del Client..."
    )
endif()