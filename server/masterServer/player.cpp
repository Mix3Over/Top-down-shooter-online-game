#include <string>
#include <netdb.h>

#include "player.hpp"
#include "common/utils.hpp"

Player::Player(int id, Connection connection): id(id), connection(connection) {}

void Player::Initialize() {
    this->isInitialized = true;
}
