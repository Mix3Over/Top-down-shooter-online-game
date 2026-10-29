#include <chrono>
#include <signal.h>

#include <vector>
#include <unordered_map>

#include "player.hpp"
#include "common/utils.hpp"
#include "common/connectionService.hpp"
#include "common/messageBuilder.hpp"

#include <fcntl.h>

enum class MessageType {
    Init,
    Heartbeat,
    PlayerCount,
    GameServerInfo,
    MessageTypeCount // Must be last
};

int heartbeatInterval = 1000;
int nextPlayerID = 0;

ConnectionService connectionService;
const MessageBuilder messageBuilder((int)MessageType::MessageTypeCount);

std::unordered_map<int, Player*> idToPlayer;
std::unordered_map<const Connection*, Player> connectionToPlayerMap;

bool isGameServerActive;
bool isGameServerActivating;
int gameServerPort;
int gameServerFd;

void ctrl_c(int);
void handleMsgStartGame(Player& player, const char* del);

void sendPlayerCountMessageToAll(){
    std::string playerCountMessage = messageBuilder.createMessage((int)MessageType::PlayerCount, idToPlayer.size());
    connectionService.sendTcpMessageToAll(playerCountMessage);
}

void onNewServerMessage(const Connection& connection, const std::string& message) {

}

void onNewConnection(Connection& connection) {
    connectionToPlayerMap[&connection] = Player(nextPlayerID++, connection);
    Player& newPlayer = connectionToPlayerMap[&connection];
    idToPlayer[newPlayer.id] = &newPlayer;

    std::string initMessage = messageBuilder.createMessage((int)MessageType::Init, newPlayer.id);
    connectionService.sendTcpMessageTo(*newPlayer.connection, initMessage);

    sendPlayerCountMessageToAll();
}

void onNewTcpMessage(const Connection& connection, const std::string& message) {
    if(!connectionToPlayerMap.contains(&connection)) {
        printf("Received message from unknown connection\n");
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
        case 'G':
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
    std::string heartbeatMessage = messageBuilder.createMessage((int)MessageType::Heartbeat);
    connectionService.sendTcpMessageToAll(heartbeatMessage);
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
        argv[1], 
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
    isGameServerActivating = true;
    
    int fds[2];
    if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) != 0) { perror("pipe2"); return; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); close(fds[0]); close(fds[1]); return; }

    if (pid == 0) {
        dup2(fds[1], STDOUT_FILENO);
        execl("./slave", "slave", (char*)nullptr);
        _exit(127);
    }

    close(fds[1]);

    connectionService.createNewConnection(fds[0], onNewServerMessage, sockaddr_in{});
    printf("[MasterServer] GameServer starte. PID:%d\n", pid);
}

void handleMsgStartGame(Player& player, const char* del){
    if(!isGameServerActive){
        startGameServer();
    }

    std::string gameServerInfoMessage = messageBuilder.createMessage((int)MessageType::GameServerInfo, gameServerPort);
    connectionService.sendTcpMessageTo(*player.connection, gameServerInfoMessage);
}
