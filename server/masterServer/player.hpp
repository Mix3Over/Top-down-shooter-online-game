#pragma once
#include <string>
#include <netdb.h>

#include "common/connectionService.hpp"

class Player {
  public:
    int id;

    std::string name;

    bool isInitialized = false;

    Connection connection;

    Player() = default;

    Player(int id, Connection connection);

    void Initialize();
};