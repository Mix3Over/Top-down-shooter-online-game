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
#include "incomingMessages.hpp"
#include "player.hpp"
#include "bullet.hpp"
#include "playerTypeConfig.hpp"

ConnectionService connectionService;
Connection* masterServerConnection;

const int bulletLifetime = 128;
const int ticksPerSecond = 32;
const int noActivePlayerTimeout = 5000; // 5 seconds

int tickCount = 0;

int nextPlayerID = 0;
int nextBulletID = 0;

std::chrono::high_resolution_clock::time_point lastActivePlayerTime;

GameType currentGameType = GameType::FFA;

std::unordered_map<int, Bullet> idToBullet;
std::unordered_map<int, Player*> idToPlayer;
std::unordered_map<const Connection*, Player> connectionToPlayerMap;

void handleMsgInit(Player& player, const incomingPlayerMessages::InitMessage& message);
void handleMsgUdpUpdate(const incomingPlayerMessages::UDP_UpdateMessage& message);
void handleSetGameModeMessage(const incomingMasterServerMessages::SetGameModeMessage& message);
void spawnBulletForPlayer(Player& player);
void playersUpdate(int deltaTime);

void onMasterServerDisconnected(const Connection& connection) {
    printf("[GameServer] Master server disconnected. Closing game server.\n");
    connectionService.closeAllConnections();
    exit(0);
}

void onNewMasterServerMessage(const Connection& connection, const std::string& message) {
    using namespace incomingMasterServerMessages;

    printf("[GameServer] Received message from master server: %s\n", message.c_str());

    auto messageParts = split(message, fieldSeparator);
    Type messageType = (Type)std::stoi(messageParts[0].data());

    switch (messageType){
        case Type::SetGameMode: {
            SetGameModeMessage setGameModeMessage;

            if(!setGameModeMessage.TryParse(messageParts)){
                printf("[GameServer] Failed to parse SetGameModeMessage: %s\n", message.c_str());
                return;
            }
            handleSetGameModeMessage(setGameModeMessage);
            break;
        }
        default:
            printf("[GameServer] Unknown message type from master server: %d Message: %s\n", (int)messageType, message.c_str());
            break;
    }
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
    using namespace incomingPlayerMessages;

    if(!connectionToPlayerMap.contains(&connection)) {
        printf("[GameServer] Received message from unknown connection\nSource: %d\nMessage: %s\n", connection.tcpFd, message.c_str());
        return;
    }

    Player& player = connectionToPlayerMap[&connection];

    Type messageType;
    std::vector<std::string_view> messageParts;
    
    splitMessage(message, messageParts, messageType);

    switch (messageType){
        case Type::Init: {
            InitMessage initMessage;

            if(!initMessage.TryParse(messageParts)){
                printf("[GameServer] Failed to parse InitMessage: %s\n", message.c_str());
                return;
            }
            handleMsgInit(player, initMessage);
            break;
        }
        default:
            printf("[GameServer] Unknown message type from player: %d Message: %s\n", (int)messageType, message.c_str());
            break;
    }
}

void onNewUdpMessage(char* messageBuffer) {
    using namespace incomingPlayerMessages;

    auto messageParts = split(messageBuffer, fieldSeparator);
    Type messageType = (Type)std::stoi(messageParts[0].data());

    switch (messageType){
        case Type::UDP_Update: {
            UDP_UpdateMessage udpUpdateMessage;

            if(!udpUpdateMessage.TryParse(messageParts)){
                printf("[GameServer] Failed to parse UDP_UpdateMessage: %s\n", messageBuffer);
                return;
            }
            handleMsgUdpUpdate(udpUpdateMessage);
            break;
        }
        default:
            printf("[GameServer] Unknown UDP message type: %d Message: %s\n", (int)messageType, messageBuffer);
            break;
    }
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

void handleSetGameModeMessage(const incomingMasterServerMessages::SetGameModeMessage& message) {
    currentGameType = (GameType)message.gameMode;
    printf("[GameServer] Game mode set to: %d\n", (int)currentGameType);
}

void handleMsgUdpUpdate(const incomingPlayerMessages::UDP_UpdateMessage& message) {
   Player* player = idToPlayer[message.playerId];
   player->inputMoveX=message.moveX;
   player->inputMoveY=message.moveY;
   player->rotation=message.rotation;
   player->isShooting=message.isShooting;
}

void handleMsgInit(Player& player, const incomingPlayerMessages::InitMessage& message) {
    currentGameType = (GameType)message.gameType;
    printf("[GameServer] Selected game type: %d\n", (int)currentGameType);

    player.Initialize(message.udpPort, message.name, message.type);
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

void spawnBulletForPlayer(Player &player)
{
    if (!player.isAlive)
        return;

    Bullet bulletData{
        .id = nextBulletID++,
        .playerId = player.id,
        .positionX = player.positionX,
        .positionY = player.positionY,
        .direction = player.rotation,
        .speed = player.bulletSpeed,
        .lifetime = bulletLifetime,
        .damage = player.damage};

    idToBullet[bulletData.id] = bulletData;

    printf("[GameServer] Player %d shot bullet %d at (%d, %d) with rotation %d\n", player.id, bulletData.id, player.positionX, player.positionY, player.rotation);

    outgoingMessages::BulletSpawn shootMessage{
        .id = bulletData.id,
        .playerId = bulletData.playerId,
        .positionX = bulletData.positionX,
        .positionY = bulletData.positionY,
        .direction = bulletData.direction};

    connectionService.sendTcpMessageToAll(shootMessage);
}

void playersUpdate(int deltaTime)
{
    auto now = std::chrono::high_resolution_clock::now();

    for (auto &[id, player] : idToPlayer)
    {
        if (!player->isInitialized || !player->isAlive)
            continue;

        float dirX = player->inputMoveX / 100.0f;
        float dirY = player->inputMoveY / 100.0f;
        float length = std::sqrt(dirX * dirX + dirY * dirY);

        if (length > 1.0f)
        {
            dirX /= length;
            dirY /= length;
        }

        player->positionX += (int)std::round(100.0f * player->movementSpeed * dirX * deltaTime / 1000.0f);
        player->positionY += std::round(100.0f * player->movementSpeed * dirY * deltaTime / 1000.0f);

        if (player->positionX > 3000)
            player->positionX = 3000;
        if (player->positionX < -3000)
            player->positionX = -3000;
        if (player->positionY > 2000)
            player->positionX = 2000;
        if (player->positionY < -2000)
            player->positionX = -2000;

        if (player->isShooting)
        {
            auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - player->lastShootTime).count();
            if (elapsedTime >= player->shootingCooldown)
            {
                player->lastShootTime = now;
                spawnBulletForPlayer(*player);
            }
        }
    }
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
    playersUpdate(deltaTime);
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

    masterServerConnection = &connectionService.createConnection(masterServerFd, onNewMasterServerMessage, onMasterServerDisconnected, sockaddr_in{}, true);
    std::string message = std::to_string(connectionService.getTcpPort());

    outgoingMasterServerMessages::GameServerInfo serverInfoMessage { 
        .tcpPort = connectionService.getTcpPort() 
    };  
    printf("[GameServer] Sending message to master server: %s\n", message.c_str());
    connectionService.sendTcpMessageTo(*masterServerConnection, serverInfoMessage);

    int cooldown = 1000 / ticksPerSecond;

    using namespace std::chrono;

    auto lastTickTime = high_resolution_clock::now();
    lastActivePlayerTime = lastTickTime;

    while (true) {
        auto duration = high_resolution_clock::now() - lastTickTime;
        auto deltaTime = duration_cast<milliseconds>(duration).count();

        if(idToPlayer.empty()){
            auto durationSinceLastActivePlayer = duration_cast<milliseconds>(high_resolution_clock::now() - lastActivePlayerTime).count();

            if(durationSinceLastActivePlayer >= noActivePlayerTimeout){
                printf("[GameServer] No active players for %d ms. Shutting down.\n", noActivePlayerTimeout);
                ctrl_c(0);
            }
        } else {
            lastActivePlayerTime = high_resolution_clock::now();
        }

        if(deltaTime >= cooldown){
            tickUpdate(deltaTime);
            tickCount++;
            lastTickTime = high_resolution_clock::now();
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
