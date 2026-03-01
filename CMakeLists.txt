set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
cmake_minimum_required(VERSION 3.16)
project(TileRace LANGUAGES C CXX)

# --- CONFIGURAZIONE GLOBALE ---
set(CMAKE_CXX_STANDARD 20)
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
# add_custom_target viene eseguito ad ogni build (debug e release),
# a differenza di file(COPY) che gira solo al configure.
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${CMAKE_BINARY_DIR}/bin/assets"
    COMMENT "Copia assets -> bin/assets"
)

# --- MODULI DEL PROGETTO ---
add_subdirectory(src/common)
add_subdirectory(src/server)
add_subdirectory(src/client)

# --- ICONA ESEGUIBILI (solo Windows) ---
# Genera app_icon.rc con il percorso assoluto all'icona, poi lo aggiunge ad
# entrambi gli eseguibili. Il percorso è assoluto per evitare problemi con le
# working-directory del compilatore di risorse.
if(WIN32)
    set(ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico")
    string(REPLACE "\\" "/" ICON_PATH "${ICON_PATH}")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/app_icon.rc.in"
        "${CMAKE_BINARY_DIR}/app_icon.rc"
        @ONLY
    )
    target_sources(TileRace        PRIVATE "${CMAKE_BINARY_DIR}/app_icon.rc")
    target_sources(TileRace_Server PRIVATE "${CMAKE_BINARY_DIR}/app_icon.rc")
endif()

# --- INSTALLAZIONE RUNTIME (solo ciò che serve per distribuire il gioco) ---
set(TILERACE_RUNTIME_COMPONENT TileRaceRuntime)

install(TARGETS TileRace TileRace_Server
  RUNTIME DESTINATION .
  COMPONENT ${TILERACE_RUNTIME_COMPONENT}
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
  DESTINATION .
  COMPONENT ${TILERACE_RUNTIME_COMPONENT}
  PATTERN "*.url" EXCLUDE
)

# --- LOGICA DI RUN ---
if(WIN32)
    add_custom_target(run
        COMMAND "$<TARGET_FILE:TileRace>"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        DEPENDS TileRace copy_assets
        COMMENT "Avvio TileRace..."
    )
    add_custom_target(run_server
        COMMAND "$<TARGET_FILE:TileRace_Server>"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        DEPENDS TileRace_Server
        COMMENT "Avvio TileRace_Server..."
    )
    # Lancia server + 2 client in finestre separate (non bloccanti tramite PowerShell Start-Process)
    add_custom_target(run_ccs
        COMMAND powershell -NoProfile -Command
            "Start-Process '$<TARGET_FILE:TileRace_Server>' -WorkingDirectory '${CMAKE_BINARY_DIR}/bin'; Start-Sleep 1; Start-Process '$<TARGET_FILE:TileRace>' -WorkingDirectory '${CMAKE_BINARY_DIR}/bin'; Start-Process '$<TARGET_FILE:TileRace>' -WorkingDirectory '${CMAKE_BINARY_DIR}/bin'"
        DEPENDS TileRace TileRace_Server copy_assets
        COMMENT "Avvio Server + 2 Client..."
    )
endif()
