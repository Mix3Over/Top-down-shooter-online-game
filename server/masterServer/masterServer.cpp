#include <chrono>
#include <signal.h>

#include <string>
#include <vector>
#include <unordered_map>

#include "player.hpp"
#include "gameServerData.hpp"
#include "common/utils.hpp"
#include "common/connectionService.hpp"
#include "common/messageBuilder.hpp"

#include <fcntl.h>

enum class SendMessage {
    Init,
    Heartbeat,
    PlayerCount,
    GameServerInfo,
    MessageTypeCount  // Must be last, used to count message types
};

enum class ReceiveMessage {
    Init,
    StartGame
};

const MessageBuilder messageBuilder((int)SendMessage::MessageTypeCount);
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
void handleMsgStartGame(Player& player, const char* del);

void sendPlayerCountMessageToAll(){
    // std::string playerCountMessage = messageBuilder.createMessage((int)SendMessage::PlayerCount, idToPlayer.size());
    // connectionService.sendTcpMessageToAll(playerCountMessage);
}

void onNewServerMessage(const Connection& connection, const std::string& message) {
    // if(!connectionToServerMap.contains(&connection)) {
    //     printf("Received onNewServerMessage from unknown connection\n");
    //     return;
    // }
    // GameServerData& gameServer = connectionToServerMap[&connection];
    printf("[M] Received message from game server: %s\n", message.c_str());

    if(!isGameServerActivating){
        printf("[M] Received message from game server but no game server is activating\n");
        return;
    }

    int tcpPort = atoi(message.c_str());

    gameServerData.isStarted = true;
    gameServerData.tcpPort = tcpPort;

    // gameServer.OnServerStarted(tcpPort);
    printf("[M] Game server started on port: %d\n", tcpPort);

    for(auto player : playersWaitingForServer){
        // ToDo: Send also IpAddress
        std::string serverInfoMessage = messageBuilder.createMessage((int)SendMessage::GameServerInfo, tcpPort);
        printf("[M] Send game server info to player: %d\n", player->id);
        connectionService.sendTcpMessageTo(*player->connection, serverInfoMessage);
    }
    playersWaitingForServer.clear();
    isGameServerActivating = false;
    isGameServerActive = true;
}

void onNewConnection(Connection& connection) {
    connectionToPlayerMap[&connection] = Player(nextPlayerID++, connection);
    Player& newPlayer = connectionToPlayerMap[&connection];
    idToPlayer[newPlayer.id] = &newPlayer;

    std::string initMessage = messageBuilder.createMessage((int)SendMessage::Init, newPlayer.id);
    connectionService.sendTcpMessageTo(*newPlayer.connection, initMessage);

    sendPlayerCountMessageToAll();
}

void onNewTcpMessage(const Connection& connection, const std::string& message) {
    if(!connectionToPlayerMap.contains(&connection)) {
        printf("[M] Received message from unknown connection\nSource: %d\nMessage: %s\n", connection.tcpFd, message.c_str());
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
        case '1':
            handleMsgStartGame(player, del);
            break;
        default:
            printf("Received unknown message\n");
            break;
    }
}

void onPlayerDisconnected(const Connection& connection) {
    if(!connectionToPlayerMap.contains(&connection)) {
        printf("Unknown connection disconnected\n");
        return;
    }

    Player& player = connectionToPlayerMap[&connection];
    
    idToPlayer.erase(player.id);
    printf("Client Id: %d, tcpFd: %d disconnected\n", player.id, player.connection->tcpFd);

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
    const char quitMsg[] = "[Server shutting down. Bye!]\n";
    connectionService.sendTcpMessageToAll(quitMsg);

    connectionService.closeAllConnections();

    printf("Closing server\n");
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

    connectionService.createNewConnection(sv[0], onNewServerMessage, sockaddr_in{});
    printf("[M] Started game server. FD: %d GameServerFD: %d\n", sv[0], sv[1]);
}

void handleMsgStartGame(Player& player, const char* del){   
    if(isGameServerActive){
        std::string serverInfoMessage = messageBuilder.createMessage((int)SendMessage::GameServerInfo, gameServerData.tcpPort);
        connectionService.sendTcpMessageTo(*player.connection, serverInfoMessage);
        return;
    }

    playersWaitingForServer.push_back(&player);

    if(!isGameServerActivating){
        isGameServerActivating = true;
        startGameServer();
    }
}
