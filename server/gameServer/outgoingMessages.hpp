#pragma once

#include "common/messages.hpp"

namespace outgoingMessages {

    enum class Type {
        Init,
        InitLocalPlayer,
        InitPlayers,
        AddRemotePlayer,
        RemoveRemotePlayer,
        BulletSpawn,
        PlayerHit,
        RemoveBullet,
        PlayerScores,
        UPD_Update,
    };

    struct Init : OutgoingMessage<Init> {
        static constexpr Type MessageType = Type::Init;

        int udpPort;
        int playerId;

        auto Fields() const { return std::tie(udpPort, playerId); }
    };

    struct InitLocalPlayer : OutgoingMessage<InitLocalPlayer> {
        static constexpr Type MessageType = Type::InitLocalPlayer;

        int positionX;
        int positionY;
        
        auto Fields() const { return std::tie(positionX, positionY); }
    };

    struct RemoveRemotePlayer : OutgoingMessage<RemoveRemotePlayer> {
        static constexpr Type MessageType = Type::RemoveRemotePlayer;

        int playerId;

        auto Fields() const { return std::tie(playerId); }
    };

    struct InitRemotePlayerRecord : OutgoingMessage<InitRemotePlayerRecord> {
        int id;
        std::string name;
        int type;
        int positionX;
        int positionY;
        int rotation;
        int health;

        auto Fields() const {
            return std::tie(id, name, type, positionX, positionY, rotation, health);
        }
    };

    struct InitRemotePlayers : OutgoingMultilineMessage<InitRemotePlayers, InitRemotePlayerRecord> { 
        static constexpr Type MessageType = Type::InitPlayers;
     };

    struct AddRemotePlayer : OutgoingMessage<AddRemotePlayer> {
        static constexpr Type MessageType = Type::AddRemotePlayer;

        int id;
        std::string name;
        int type;
        int positionX;
        int positionY;
        int rotation;
        int health;

        auto Fields() const { return std::tie(id, name, type, positionX, positionY, rotation, health); }
    };

    struct BulletSpawn : OutgoingMessage<BulletSpawn> {
        static constexpr Type MessageType = Type::BulletSpawn;

        int id;
        int playerId;
        int positionX;
        int positionY;
        int direction;

        auto Fields() const { return std::tie(id, playerId, positionX, positionY, direction); }
    };

    struct PlayerHit : OutgoingMessage<PlayerHit> {
        static constexpr Type MessageType = Type::PlayerHit;

        int playerId;
        int bulletId;
        int health;
        bool isAlive;

        auto Fields() const { return std::tie(playerId, bulletId, health, isAlive); }
    };

    struct RemoveBullet : OutgoingMessage<RemoveBullet> {
        static constexpr Type MessageType = Type::RemoveBullet;

        int id;

        auto Fields() const { return std::tie(id); }
    };

    struct PlayerScoreRecord : OutgoingMessage<PlayerScoreRecord> {
        int id;
        int points;

        auto Fields() const { return std::tie(id, points); }
    };

    struct PlayerScores : OutgoingMultilineMessage<PlayerScores, PlayerScoreRecord> { 
        static constexpr Type MessageType = Type::PlayerScores;
     };

    struct UPD_UpdateRecord : OutgoingMessage<UPD_UpdateRecord> {
        int id;
        int positionX;
        int positionY;
        int rotation;

        auto Fields() const { return std::tie(id, positionX, positionY, rotation); }
    };

    struct UPD_Update : OutgoingMultilineMessage<UPD_Update, UPD_UpdateRecord> { 
        static constexpr Type MessageType = Type::UPD_Update;
    };
}

namespace outgoingMasterServerMessages {
    enum class Type {
        GameServerInfo,
    };

    struct GameServerInfo : OutgoingMessage<GameServerInfo> {
        static constexpr Type MessageType = Type::GameServerInfo;

        int tcpPort;

        auto Fields() const { return std::tie(tcpPort); }
    };
}
