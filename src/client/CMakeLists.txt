add_executable(TileRace_Client main.cpp)

# Configura SOLO il Client
target_include_directories(TileRace_Client PRIVATE "${CMAKE_SOURCE_DIR}/vendored/enet/include")

target_link_libraries(TileRace_Client PRIVATE 
    common_logic 
    raylib
    enet
)

install(TARGETS TileRace_Client DESTINATION .)