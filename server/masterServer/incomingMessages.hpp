#pragma once

#include "common/messages.hpp"

namespace incomingPlayerMessages {

    enum class Type {
        StartGame,
    };

    struct StartGameMessage : IncomingMessage<StartGameMessage> {
        int gameMode = 0;
        auto Fields() { return std::tie(gameMode); }
    };
}

namespace incomingGameServerMessages {

    enum class Type {
        ServerStarted,
    };

    struct GameServerStarted : IncomingMessage<GameServerStarted> {
        int tcpPort = 0;

        auto Fields() { return std::tie(tcpPort); }
    };
}