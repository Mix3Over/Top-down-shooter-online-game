cmake_minimum_required(VERSION 3.10)
project(TestSteamNetworkingSockets LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# GameNetworkingSockets v1.4.1 does not look these up by itself, so do it first
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(GameNetworkingSockets CONFIG REQUIRED)

add_executable(gnsEcho echo.cpp)
target_link_libraries(gnsEcho PRIVATE GameNetworkingSockets::GameNetworkingSockets)
