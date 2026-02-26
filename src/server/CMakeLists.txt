add_executable(TileRace_Server main.cpp)

# Configura SOLO il Server
target_include_directories(TileRace_Server PRIVATE "${CMAKE_SOURCE_DIR}/vendored/enet/include")

target_link_libraries(TileRace_Server PRIVATE 
    common_logic 
    enet
)

install(TARGETS TileRace_Server DESTINATION .)