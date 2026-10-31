cmake_minimum_required(VERSION 3.10)

project(SK2_Projekt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(masterServer
    masterServer/masterServer.cpp
    masterServer/player.cpp
    common/connectionService.cpp
    common/messageBuilder.cpp
    common/utils.cpp
)

add_executable(gameServer
    gameServer/gameServer.cpp
    gameServer/player.cpp
    common/connectionService.cpp
    common/messageBuilder.cpp
    common/utils.cpp
)

target_include_directories(masterServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(gameServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})