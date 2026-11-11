#pragma once

#include "common/messages.hpp"

namespace outgoingMessages {

    enum class Type {
        PlayerInit,
        GameServerInfo,
        Heartbeat,
        PlayerCount,
    };

    struct GameServerInfo : OutgoingMessage<GameServerInfo> {
        static constexpr Type MessageType = Type::GameServerInfo;

        int tcpPort = 0;

        auto Fields() const { return std::tie(tcpPort); }
    };

    struct PlayerInit : OutgoingMessage<PlayerInit> {
        static constexpr Type MessageType = Type::PlayerInit;

        int playerId = 0;

        auto Fields() const { return std::tie(playerId); }
    };
}


