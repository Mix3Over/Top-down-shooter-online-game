#include <chrono>
#include <cmath>
#include <signal.h>

#include <string>
#include <vector>
#include <unordered_map>

#include "common/utils.hpp"
#include "common/connectionService.hpp"
#include "common/GameType.hpp"
#include "outgoingMessages.hpp"
#include "player.hpp"
#include "bullet.hpp"

enum class SendMessage {
    Init,
    InitLocalPlayer,
    InitPlayers,
    AddRemotePlayer,
    RemoveRemotePlayer,
    BulletSpawn,
    PlayerHit,
    RemoveBullet,
    PlayerScores,
};

enum class ReceiveMessage {
    Init,
    StartGame
};

ConnectionService connectionService;
Connection* masterServerConnection;

const int bulletLifetime = 128;
const int ticksPerSecond = 32;

int tickCount = 0;

int nextPlayerID = 0;
int nextBulletID = 0;

GameType currentGameType = GameType::FFA;

std::unordered_map<int, Bullet> idToBullet;
std::unordered_map<int, Player*> idToPlayer;
std::unordered_map<const Connection*, Player> connectionToPlayerMap;

void handleMsgInit(Player& player, const char* del);
void handleMsgShoot(Player& player, const char* del);

void onNewMasterServerMessage(const Connection& connection, const std::string& message) {
    printf("[GameServer] Received message from master server: %s\n", message.c_str());
    int mode = atoi(message.c_str());
    printf("[GameServer] Game mode set to: %d\n", (int)currentGameType);
}

void onNewConnection(Connection& connection) {
    connectionToPlayerMap[&connection] = Player(nextPlayerID++, connection);
    Player& newPlayer = connectionToPlayerMap[&connection];
    idToPlayer[newPlayer.id] = &newPlayer;

    outgoingMessages::Init initMessage{ 
        .udpPort = connectionService.getUdpPort(), 
        .playerId = newPlayer.id 
    };

    connectionService.sendTcpMessageTo(*newPlayer.connection, initMessage);
}

void onNewTcpMessage(const Connection& connection, const std::string& message) {
    if(!connectionToPlayerMap.contains(&connection)) {
        printf("[GameServer] Received message from unknown connection\nSource: %d\nMessage: %s\n", connection.tcpFd, message.c_str());
        return;
    }

    Player& player = connectionToPlayerMap[&connection];
    
    std::vector<char> msgBuf(message.begin(), message.end());
    msgBuf.push_back('\0');

    const char* del = ";";
    char *t = strtok(msgBuf.data(), del);
    
    if (!t) return;

    char messageType = t[0];

    switch (messageType){
        case 'I':
            handleMsgInit(player, del);
            break;
        case 'S':
            handleMsgShoot(player, del);
            break;
        default:
            printf("[GameServer] Unknown message type: %c\n", messageType);
            break;
    }
}

void onNewUdpMessage(char* messageBuffer) {
    const char *del = ";";
    int playerId, positionX, positionY, rotation;

    char *t = strtok(messageBuffer, del);

    if(t == nullptr)       
        return;        
    playerId = atoi(t);
    t = strtok(nullptr, del);

    if(t == nullptr)       
        return;        
    positionX = atoi(t);
    t = strtok(nullptr, del);

    if(t == nullptr)       
        return;        
    positionY = atoi(t);
    t = strtok(nullptr, del);

    if(t == nullptr)       
        return;        
    rotation = atoi(t);

    idToPlayer[playerId]->positionX = positionX;
    idToPlayer[playerId]->positionY = positionY;
    idToPlayer[playerId]->rotation = rotation;
}

void onPlayerDisconnected(const Connection& connection) {
    if(!connectionToPlayerMap.contains(&connection)) {
        printf("[GameServer] Unknown connection disconnected\n");
        return;
    }

    Player* player = &connectionToPlayerMap[&connection];
    idToPlayer.erase(player->id);

    printf("[GameServer] Client Id: %d, tcpFd: %d disconnected\n", player->id, player->connection->tcpFd);

    outgoingMessages::RemoveRemotePlayer removeMessage{ .playerId = player->id };

    connectionToPlayerMap.erase(&connection);
    connectionService.sendTcpMessageToAll(removeMessage);
}

void handleMsgInit(Player& player, const char* del) {
    char *t = strtok(nullptr, del);
    int playerUdpPort = atoi(t);
    t = strtok(nullptr, del);
    std::string name = std::string(t);
    t = strtok(nullptr, del);
    int type = atoi(t);
    t = strtok(nullptr, del);
    int damage = atoi(t);
    t = strtok(nullptr, del);
    int bulletSpeed = atoi(t);
    t = strtok(nullptr, del);
    int hp = atoi(t);

    t = strtok(nullptr, del);
    if (t != nullptr){
        currentGameType = (GameType)atoi(t);
        printf("[GameServer] Selected game type: %d\n", (int)currentGameType);
    }

    player.Initialize(playerUdpPort, name, type, damage, bulletSpeed, hp);
    printf("[GameServer] Player initialized: %d\n", player.isAlive);

    outgoingMessages::InitLocalPlayer initMessage{ .positionX = player.positionX, .positionY = player.positionY };
    connectionService.sendTcpMessageTo(*player.connection, initMessage);
        
    outgoingMessages::InitRemotePlayers initRemotePlayersMessage;
    for (const auto& [id, player] : idToPlayer) {
        if (!player->isAlive) continue;

        initRemotePlayersMessage.addRecord(outgoingMessages::InitRemotePlayerRecord { 
            .id = player->id, 
            .name = player->name,
            .type = player->type,
            .positionX = player->positionX,
            .positionY = player->positionY,
            .rotation = player->rotation,
            .health = player->health
        });
    }
    connectionService.sendTcpMessageTo(*player.connection, initRemotePlayersMessage);

    outgoingMessages::AddRemotePlayer addPlayerMessage{ 
        .id = player.id,
        .name = player.name,
        .type = player.type,
        .positionX = player.positionX,
        .positionY = player.positionY,
        .rotation = player.rotation,
        .health = player.health
    };

    connectionService.sendTcpMessageToAll(addPlayerMessage);
}

void handleMsgShoot(Player& player, const char* del) {
    char *t = strtok(nullptr, del);
    int positionX = atoi(t);
    t = strtok(nullptr, del);
    int positionY = atoi(t);
    t = strtok(nullptr, del);
    int rotation = atoi(t);

    if(!player.isAlive)
        return;
    
    Bullet bulletData {
        .id = nextBulletID++,
        .playerId = player.id,
        .positionX = positionX,
        .positionY = positionY,
        .direction = rotation,
        .speed = player.bulletSpeed,
        .lifetime = bulletLifetime,
        .damage = player.damage
    };

    idToBullet[bulletData.id] = bulletData;

    printf("[GameServer] Player %d shot bullet %d at (%d, %d) with rotation %d\n", player.id, bulletData.id, positionX, positionY, rotation);
    
    outgoingMessages::BulletSpawn shootMessage{ 
        .id = bulletData.id, 
        .playerId = bulletData.playerId, 
        .positionX = bulletData.positionX, 
        .positionY = bulletData.positionY, 
        .direction = bulletData.direction 
    };

    connectionService.sendTcpMessageToAll(shootMessage);
}

void bulletsUpdate(int deltaTime) {
    for (auto it = idToBullet.begin(); it != idToBullet.end(); ) {
        
        auto& bulletData = it->second; 
        bool bulletRemoved = false;

        // Aktualizacja pozycji
        bulletData.positionX += 100 * bulletData.speed * cos(bulletData.direction * M_PI / 180.0) * deltaTime / 1000;
        bulletData.positionY += 100 * bulletData.speed * sin(bulletData.direction * M_PI / 180.0) * deltaTime / 1000;

        // Sprawdzanie kolizji z graczami
        for (const auto& playerEntry : idToPlayer) {
            const auto& player = playerEntry.second;

            if (player->id == bulletData.playerId || !player->isAlive)
                continue; 

            long dx = bulletData.positionX - player->positionX;
            long dy = bulletData.positionY - player->positionY;
            long distanceSquared = dx * dx + dy * dy;
            long collisionDistance = 65; 

            if (distanceSquared <= collisionDistance * collisionDistance) {
                
                idToPlayer[player->id]->health -= bulletData.damage;
                
                printf("[GameServer] Player %d hit by bullet %d. Health: %d\n",
                       player->id, bulletData.id, idToPlayer[player->id]->health);

                
                if (idToPlayer[player->id]->health <= 0) {
                    printf("[GameServer] Player %d has died.\n", player->id);

                    idToPlayer[player->id]->isAlive = false;
                    idToPlayer[bulletData.playerId]->points += 100;
                    idToPlayer[player->id]->health = 0;
                    idToPlayer[player->id]->points = 0;
                }

                outgoingMessages::PlayerHit hitMessage{ 
                    .playerId = player->id, 
                    .bulletId = bulletData.id, 
                    .health = idToPlayer[player->id]->health, 
                    .isAlive = idToPlayer[player->id]->isAlive 
                };

                connectionService.sendTcpMessageToAll(hitMessage);

                it = idToBullet.erase(it); 
                bulletRemoved = true;
                break; 
            }
        }
        
        if (bulletRemoved) 
            continue;

        bulletData.lifetime -= 1;

        if (bulletData.lifetime <= 0) {
            outgoingMessages::RemoveBullet removeMessage{ .id = bulletData.id };
            connectionService.sendTcpMessageToAll(removeMessage);

            it = idToBullet.erase(it);
        } else {
            ++it;
        }
    }
}

void updatePlayersScore(){
    for(const auto& i : idToPlayer){
        if(i.second->isAlive)
        idToPlayer[i.first]->points += 1;
    }

    outgoingMessages::PlayerScores scoreMessage;
    for(const auto& [id, player] : idToPlayer){
        if(!player->isInitialized)
            continue;
        scoreMessage.addRecord(outgoingMessages::PlayerScoreRecord { 
            .id = player->id, 
            .points = player->points 
        });
    }

    connectionService.sendTcpMessageToAll(scoreMessage);
}

void sendUpdateToPlayers(){
    outgoingMessages::UPD_Update updateMessage;
    for(const auto& [id, player] : idToPlayer){
        if(!player->isAlive)
            continue;

        updateMessage.addRecord(outgoingMessages::UPD_UpdateRecord { 
            .id = player->id, 
            .positionX = player->positionX, 
            .positionY = player->positionY, 
            .rotation = player->rotation 
        });
    }

    connectionService.sendUdpMessageToAll(updateMessage);
}

void tickUpdate(int deltaTime){
    sendUpdateToPlayers();
    if(tickCount % ticksPerSecond == 0){
        updatePlayersScore();
        tickCount = 0;
    }
    bulletsUpdate(deltaTime);
}

// handles SIGINT
void ctrl_c(int);

int main(int argc, char **argv) {
    if (argc != 2) 
        error(1, 0, "Usage: %s <tcp-port>", argv[0]);

    int masterServerFd = atoi(argv[1]);
    printf("arg1 %d\n", masterServerFd);

    srand(time(0));

    // set up ctrl+c handler for a graceful exit
    signal(SIGINT, ctrl_c);
    // prevent dead sockets from throwing pipe errors on write
    signal(SIGPIPE, SIG_IGN);

    connectionService = ConnectionService(
        RANDOM_PORT, 
        onNewConnection, 
        onNewTcpMessage,
        onPlayerDisconnected,

        true, 
        onNewUdpMessage
    );

    masterServerConnection = &connectionService.createConnection(masterServerFd, onNewMasterServerMessage, sockaddr_in{}, true);
    std::string message = std::to_string(connectionService.getTcpPort());

    outgoingMasterServerMessages::GameServerInfo serverInfoMessage { 
        .tcpPort = connectionService.getTcpPort() 
    };  
    printf("[GameServer] Sending message to master server: %s\n", message.c_str());
    connectionService.sendTcpMessageTo(*masterServerConnection, serverInfoMessage);

    int cooldown = 1000 / ticksPerSecond;
    auto lastTickTime = std::chrono::high_resolution_clock::now();

    while (true) {
        auto duration = std::chrono::high_resolution_clock::now() - lastTickTime;
        auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        if(deltaTime >= cooldown){
            tickUpdate(deltaTime);
            tickCount++;
            lastTickTime = std::chrono::high_resolution_clock::now();
            continue;
        }

        int timeLeftToNextTick = cooldown - deltaTime;

        connectionService.listenForEvents(timeLeftToNextTick);
    }
}

void ctrl_c(int) {
    connectionService.closeAllConnections();

    printf("[GameServer] Closing server\n");
    exit(0);
}
