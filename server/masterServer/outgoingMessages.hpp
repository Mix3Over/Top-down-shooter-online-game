#pragma once

#include "common/messages.hpp"

namespace outgoingMessages {

    enum class Type {
        Heartbeat,
        PlayerInfo,
        PlayersCount,
        GameServerInfo,
    };

    struct GameServerInfo : OutgoingMessage<GameServerInfo> {
        static constexpr Type MessageType = Type::GameServerInfo;

        int tcpPort = 0;

        auto Fields() const { return std::tie(tcpPort); }
    };

    struct PlayerInit : OutgoingMessage<PlayerInit> {
        static constexpr Type MessageType = Type::PlayerInfo;

        int playerId = 0;

        auto Fields() const { return std::tie(playerId); }
    };

    struct PlayersCount : OutgoingMessage<PlayersCount> {
        static constexpr Type MessageType = Type::PlayersCount;

        int playersCount = 0;

        auto Fields() const { return std::tie(playersCount); }
    };

    struct Hearbeat : OutgoingMessage<Hearbeat> {
        static constexpr Type MessageType = Type::Heartbeat;

        auto Fields() const { return std::tie(); }
    };
}


