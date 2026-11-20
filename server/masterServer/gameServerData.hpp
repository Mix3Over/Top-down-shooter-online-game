#pragma once

#include "common/connectionService.hpp"
#include "common/GameType.hpp"

class GameServerData {
  public:
    int writeFd = -1;
    
    Connection* connection;
    GameType gameType;

    int tcpPort;
    bool isStarted = false;

    GameServerData() = default;
    GameServerData(int writeFd, Connection& connection, GameType gameType);

    void OnServerStarted(int tcpPort);
};