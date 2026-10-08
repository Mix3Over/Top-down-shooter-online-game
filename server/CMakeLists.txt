cmake_minimum_required(VERSION 3.10)

project(SK2_Projekt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(server
    server.cpp
    connectionService.cpp
    player.cpp
    utils.cpp
)