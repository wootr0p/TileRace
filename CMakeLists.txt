cmake_minimum_required(VERSION 3.16)
project(TileRace)

# Standard C++ per tutti i progetti
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- CONFIGURAZIONE DIRECTORY DI OUTPUT ---
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# --- FIX PER WINDOWS ---
if(WIN32)
    add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# --- LIBRERIE VENDORED ---
add_subdirectory(vendored/raylib EXCLUDE_FROM_ALL)
add_subdirectory(vendored/enet EXCLUDE_FROM_ALL)

# --- I NOSTRI MODULI ---
add_subdirectory(src/common)
add_subdirectory(src/client)
add_subdirectory(src/server)

# Copia degli assets per la build
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" 
     DESTINATION "${CMAKE_BINARY_DIR}/bin")

# Installa gli assets per la distribuzione
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION .)

# --- CUSTOM TARGET PER LANCIARE IL GIOCO ---
if(WIN32)
    add_custom_target(run
        COMMAND start cmd /c "$<TARGET_FILE:TileRace_Server>"
        COMMAND ${CMAKE_COMMAND} -E sleep 1
        COMMAND "$<TARGET_FILE:TileRace_Client>"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        DEPENDS TileRace_Server TileRace_Client
        COMMENT "Avvio del Server e del Client..."
    )
else()
    add_custom_target(run
        COMMAND "$<TARGET_FILE:TileRace_Server>" &
        COMMAND ${CMAKE_COMMAND} -E sleep 1
        COMMAND "$<TARGET_FILE:TileRace_Client>"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        DEPENDS TileRace_Server TileRace_Client
        COMMENT "Avvio del Server e del Client..."
    )
endif()