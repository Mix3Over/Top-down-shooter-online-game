#include <chrono>
#include <signal.h>

#include <string>
#include <vector>
#include <unordered_map>

#include "player.hpp"
#include "gameServerData.hpp"
#include "common/utils.hpp"
#include "common/connectionService.hpp"
#include "outgoingMessages.hpp"
#include "incomingMessages.hpp"

#include <fcntl.h>

ConnectionService connectionService;

int heartbeatInterval = 1000;
int nextPlayerID = 0;

std::unordered_map<int, Player*> idToPlayer;
std::unordered_map<const Connection*, Player> connectionToPlayerMap;

// std::unordered_map<const Connection*, GameServerData> connectionToServerMap;
GameServerData gameServerData;

std::vector<Player*> playersWaitingForServer;

bool isGameServerActive;
bool isGameServerActivating;

void ctrl_c(int);
void handleMsgStartGame(Player& player, const incomingPlayerMessages::StartGameMessage& message);

void handleServerStartedMessage(const incomingGameServerMessages::GameServerStarted& message) {
    gameServerData.isStarted = true;
    gameServerData.tcpPort = message.tcpPort;

    printf("[MasterServer] Game server started on port: %d\n", message.tcpPort);

    for(auto player : playersWaitingForServer){
        // ToDo: Send also IpAddress
        outgoingMessages::GameServerInfo serverInfoMessage { .tcpPort = message.tcpPort };
        printf("[MasterServer] Send game server info to player: %d\n", player->id);
        connectionService.sendTcpMessageTo(*player->connection, serverInfoMessage);
    }
    playersWaitingForServer.clear();
    isGameServerActivating = false;
    isGameServerActive = true;
}

std::vector<std::string_view> split(std::string_view s, char delim) {
    std::vector<std::string_view> result;
    size_t start = 0, pos;
    while ((pos = s.find(delim, start)) != std::string_view::npos) {
        result.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    result.push_back(s.substr(start));
    return result;
}

void sendPlayerCountMessageToAll(){
    // std::string playerCountMessage = messageBuilder.createMessage((int)SendMessage::PlayerCount, idToPlayer.size());
    // connectionService.sendTcpMessageToAll(playerCountMessage);
}

void onNewServerMessage(const Connection& connection, const std::string& message) {
    using namespace incomingGameServerMessages;

    printf("[MasterServer] Received message from game server: %s\n", message.c_str());

    auto messageParts = split(message, ';');
    Type messageType = (Type)std::stoi(messageParts[0].data());

    switch (messageType){
        case Type::ServerStarted: {
            GameServerStarted startedMessage;

            if(!startedMessage.TryParse(messageParts)){
                printf("[MasterServer] Failed to parse GameServerStarted: %s\n", message.c_str());
                return;
            }
            handleServerStartedMessage(startedMessage);
            break;
        }
        default:
            printf("[MasterServer] Unknown message type from game server: %d Message: %s\n", (int)messageType, message.c_str());
            break;
    }
}

void onNewConnection(Connection& connection) {
    connectionToPlayerMap[&connection] = Player(nextPlayerID++, connection);
    Player& newPlayer = connectionToPlayerMap[&connection];
    idToPlayer[newPlayer.id] = &newPlayer;

    outgoingMessages::PlayerInit playerInitMessage { .playerId = newPlayer.id };
    printf("[MasterServer] New player connected: %d, tcpFd: %d\n", newPlayer.id, newPlayer.connection->tcpFd);
    connectionService.sendTcpMessageTo(*newPlayer.connection, playerInitMessage);
    printf("[MasterServer] Init message sent to player: %d, tcpFd: %d\n", newPlayer.id, newPlayer.connection->tcpFd);

    sendPlayerCountMessageToAll();
}

void onNewTcpMessage(const Connection& connection, const std::string& message) {
    using namespace incomingPlayerMessages;


    if(!connectionToPlayerMap.contains(&connection)) {
        printf("[MasterServer] Received message from unknown connection\nSource: %d\nMessage: %s\n", connection.tcpFd, message.c_str());
        return;
    }

    Player& player = connectionToPlayerMap[&connection];

    auto messageParts = split(message, ';');
    Type messageType = (Type)std::stoi(messageParts[0].data());

    switch (messageType){
        case Type::StartGame: {
            StartGameMessage startGameMessage;

            if(!startGameMessage.TryParse(messageParts)){
                printf("[MasterServer] Failed to parse StartGameMessage: %s\n", message.c_str());
                return;
            }
            handleMsgStartGame(player, startGameMessage);
            break;
        }
        default:
            printf("[MasterServer] Unknown message type from player: %d Message: %s\n", (int)messageType, message.c_str());
            break;
    }
}

void onPlayerDisconnected(const Connection& connection) {
    if(!connectionToPlayerMap.contains(&connection)) {
        printf("[MasterServer] Unknown connection disconnected\n");
        return;
    }

    Player& player = connectionToPlayerMap[&connection];
    
    idToPlayer.erase(player.id);
    printf("[MasterServer] Client Id: %d, tcpFd: %d disconnected\n", player.id, player.connection->tcpFd);

    connectionToPlayerMap.erase(&connection);

    sendPlayerCountMessageToAll();
}

void sendHeartbeat() {
    // std::string heartbeatMessage = messageBuilder.createMessage((int)SendMessage::Heartbeat);
    // connectionService.sendTcpMessageToAll(heartbeatMessage);
}

int main(int argc, char **argv) {
    if (argc != 2) 
        error(1, 0, "Usage: %s <tcp-port>", argv[0]);

    int tcpPort = atoi(argv[1]);

    if (tcpPort > 65535 || tcpPort < 1024)
        error(1, 0, "Ports must be in range 1024-65535");

    srand(time(nullptr));

    // set up ctrl+c handler for a graceful exit
    signal(SIGINT, ctrl_c);
    // prevent dead sockets from throwing pipe errors on write
    signal(SIGPIPE, SIG_IGN);

    connectionService = ConnectionService(
        tcpPort, 
        onNewConnection, 
        onNewTcpMessage,
        onPlayerDisconnected,

        false, 
        nullptr
    );

    auto lastHeartbeatTime = std::chrono::high_resolution_clock::now();

    while (true) {
        auto duration = std::chrono::high_resolution_clock::now() - lastHeartbeatTime;
        auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        if(deltaTime >= heartbeatInterval){
            sendHeartbeat();
            lastHeartbeatTime = std::chrono::high_resolution_clock::now();
            continue;
        }

        connectionService.listenForEvents(heartbeatInterval - deltaTime);
    }
}

void ctrl_c(int) {
    connectionService.closeAllConnections();

    printf("[MasterServer] Closing server\n");
    exit(0);
}

void startGameServer(){
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) != 0) { 
        perror("socketpair"); return; 
    }

    fcntl(sv[0], F_SETFL, fcntl(sv[0], F_GETFL) | O_NONBLOCK);

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); close(sv[0]); close(sv[1]); return; }

    if (pid == 0) {
        close(sv[0]);
        fcntl(sv[1], F_SETFD, fcntl(sv[1], F_GETFD) & ~FD_CLOEXEC);

        std::string fdstr = std::to_string(sv[1]);
        execl("./build/gameServer", "gameServer", fdstr.c_str(), (char*)nullptr);
        _exit(127);
    }

    close(sv[1]);

    connectionService.createConnection(sv[0], onNewServerMessage, sockaddr_in{}, true);
    printf("[MasterServer] Started game server. FD: %d GameServerFD: %d\n", sv[0], sv[1]);
}

void handleMsgStartGame(Player& player, const incomingPlayerMessages::StartGameMessage& message) {
    printf("[MasterServer] Handling start game message for player %d\n", player.id);
    if(isGameServerActive){
        outgoingMessages::GameServerInfo gameServerInfoMessage { .tcpPort = gameServerData.tcpPort };
        printf("[MasterServer] Sending game server info to player %d. Message: %s\n", player.id, gameServerInfoMessage.Build().c_str());
        connectionService.sendTcpMessageTo(*player.connection, gameServerInfoMessage);
        return;
    }

    playersWaitingForServer.push_back(&player);

    if(!isGameServerActivating){
        isGameServerActivating = true;
        startGameServer();
    }
}
