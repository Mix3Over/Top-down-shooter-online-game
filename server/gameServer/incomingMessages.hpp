#pragma once

#include "common/messages.hpp"

namespace incomingPlayerMessages {

    enum class Type {
        Init,
        Shoot,
        UDP_Update,
    };

    struct InitMessage : IncomingMessage<InitMessage> {
        int udpPort = 0;
        std::string name;
        int type = 0;
        int damage = 0;
        int bulletSpeed = 0;
        int hp = 0;
        int gameType = 0;

        auto Fields() { return std::tie(udpPort, name, type, damage, bulletSpeed, hp, gameType); }
    };

    struct ShootMessage : IncomingMessage<ShootMessage> {
        int positionX = 0;
        int positionY = 0;
        int rotation = 0;

        auto Fields() { return std::tie(positionX, positionY, rotation); }
    };

    struct UDP_UpdateMessage : IncomingMessage<UDP_UpdateMessage> {
        int playerId = 0;
        int positionX = 0;
        int positionY = 0;
        int rotation = 0;

        auto Fields() { return std::tie(playerId, positionX, positionY, rotation); }
    };
}

namespace incomingMasterServerMessages {

    enum class Type {
        SetGameMode,
    };

    struct SetGameModeMessage : IncomingMessage<SetGameModeMessage> {
        int gameMode = 0;

        auto Fields() { return std::tie(gameMode); }
    };
}

