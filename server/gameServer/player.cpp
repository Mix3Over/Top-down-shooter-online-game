#include <string>
#include <netdb.h>

#include "player.hpp"
#include "common/utils.hpp"
#include "playerTypeConfig.hpp"

Player::Player(int id, Connection& connection): id(id), connection(&connection) {}

void Player::Initialize(int udpPort, std::string name, int type) {
    connection->udpAddress.sin_port = htons(udpPort);
    connection->isUdpInitialized = true;

    int typeChoosed = (type >= 0 && type < PlayerTypeConfigs.size()) ? type:0;
    const PlayerTypeConfig& config = PlayerTypeConfigs[typeChoosed];

    this->name = name;
    this->type = typeChoosed;
    this->damage = config.bulletDamage;
    this->health = config.startingHealth;
    this->bulletSpeed = config.bulletSpeed;
    this->movementSpeed = config.movementSpeed;
    this->shootingCooldown = config.shootingCooldown;
    this->bulletSize = config.bulletSize;
    this->points = 0;

    this->inputMoveX = 0;
    this->inputMoveY = 0;
    this->isShooting = false;
    this->lastShootTime = std::chrono::high_resolution_clock::now()-std::chrono::milliseconds(config.shootingCooldown);

    this->isInitialized = true;
    this->isAlive = true;

    this->positionX = getRandomNumberInRange(0, 1000);
    this->positionY = getRandomNumberInRange(0, 1000);
}
