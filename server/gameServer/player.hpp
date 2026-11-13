#pragma once
#include <string>
#include <netdb.h>
#include <chrono>

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

    float movementSpeed = 0.0f;
    int shootingCooldown = 500;
    float bulletSize = 0.2f;
    std::chrono::high_resolution_clock::time_point lastShootTime{};

    int inputMoveX = 0;
    int inputMoveY = 0;
    bool isShooting = false;

    Connection* connection;

    Player() = default;

    Player(int id, Connection& connection);

    void Initialize(int udpPort, std::string name, int type);
};