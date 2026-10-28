#pragma once
#include <string>
#include <netdb.h>

#include "common/connectionService.hpp"

class Player {
  public:
    int id;

    std::string name;

    bool isInitialized = false;
    bool isAlive = false;

    int type = 0;
    int damage = 0;
    int bulletSpeed = 0;
    int health = 0;
    int points = 0;

    int positionX = 0;
    int positionY = 0;
    int rotation = 0;

    Connection* connection;

    Player() = default;

    Player(int id, Connection& connection);

    void Initialize(int udpPort, std::string name, int type, int damage, int bulletSpeed, int hp);
};