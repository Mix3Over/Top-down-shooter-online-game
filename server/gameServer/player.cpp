#include <string>
#include <netdb.h>

#include "player.hpp"
#include "common/utils.hpp"

Player::Player(int id, Connection connection): id(id), connection(connection) {}

void Player::Initialize(int udpPort, std::string name, int type, int damage, int bulletSpeed, int hp) {
    connection.udpAddress.sin_port = htons(udpPort);
    connection.isUdpInitialized = true;

    this->name = name;
    this->type = type;
    this->damage = damage;
    this->health = hp;
    this->bulletSpeed = bulletSpeed;

    this->isInitialized = true;
    this->isAlive = true;

    this->positionX = getRandomNumberInRange(0, 1000);
    this->positionY = getRandomNumberInRange(0, 1000);
}
