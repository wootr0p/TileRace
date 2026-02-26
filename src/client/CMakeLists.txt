add_executable(TileRace_Client main.cpp)

# Configura SOLO il Client
target_include_directories(TileRace_Client PRIVATE "${CMAKE_SOURCE_DIR}/vendored/enet/include")

target_link_libraries(TileRace_Client PRIVATE 
    common_logic 
    SDL3::SDL3 
    enet
)

install(TARGETS TileRace_Client DESTINATION .)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets" DESTINATION .)

# Installa solo la libreria dinamica (DLL/SO/DYLIB) e ignora i file di importazione (.lib/.dll.a)
install(TARGETS SDL3-shared 
    RUNTIME DESTINATION . # Per le .dll su Windows
    LIBRARY DESTINATION . # Per i .so su Linux e .dylib su Mac
    # Omettendo ARCHIVE, i file .lib/.dll.a non verranno installati affatto
)