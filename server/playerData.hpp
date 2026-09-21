#pragma once
#include <string>
#include <netdb.h>

class PlayerData {
  public:
    PlayerData() = default;

    PlayerData(int id, int fd, sockaddr_storage &clientAddr): playerId(id), tcpFd(fd) {
        playerAddr.sin_addr = ((sockaddr_in *)&clientAddr)->sin_addr;
        playerAddr.sin_family = AF_INET;
    }

    int playerId;
    int tcpFd;
    sockaddr_in playerAddr{};

    std::string name;
    std::string inputBuffer;

    bool isInitialized = false;
    bool isAlive = false;
    int health = 0;

    int positionX = 0;
    int positionY = 0;
    int rotation = 0;
    int type = 0;
    int damage = 0;
    int bulletSpeed = 0;
    int points = 0;
};