# Aggiungi il .cpp qui
add_library(common_logic STATIC 
    Protocol.h
    GameLogic.cpp 
)

target_include_directories(common_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})