#include <chrono>
#include <signal.h>

#include <vector>
#include <unordered_map>

#include "player.hpp"
#include "common/utils.hpp"
#include "common/connectionService.hpp"

int heartbeatInterval = 1000;
int nextPlayerID = 0;

ConnectionService connectionService;

std::unordered_map<int, Player> idToPlayer;
std::unordered_map<Connection, Player> connectionToPlayerMap;

void ctrl_c(int);

void onNewConnection(Connection& connection) {
    Player newPlayer = Player(nextPlayerID++, connection);
    connectionToPlayerMap[connection] = newPlayer;

    std::string s = std::to_string(connectionService.getUdpPort()) + ";" + std::to_string(newPlayer.id) + "~";
    connectionService.sendTcpMessageTo(newPlayer.connection, s);
}

void onNewTcpMessage(const Connection& connection, const std::string& message) {
    if(!connectionToPlayerMap.contains(connection)) {
        printf("Received message from unknown connection\n");
        return;
    }

    Player& player = connectionToPlayerMap[connection];
    
    std::vector<char> msgBuf(message.begin(), message.end());
    msgBuf.push_back('\0');

    const char* del = ";";
    char *t = strtok(msgBuf.data(), del);
    
    if (!t) return;

    char messageType = t[0];

    switch (messageType){
        case 'I':
            // handleMsgInit(player, del);
            break;
        case 'S':
            // handleMsgShoot(player, del);
            break;
        default:
            break;
    }
}

void onPlayerDisconnected(const Connection& connection) {
    if(!connectionToPlayerMap.contains(connection)) {
        printf("Unknown connection disconnected\n");
        return;
    }

    connectionToPlayerMap.erase(connection);

    Player player = connectionToPlayerMap[connection];
    idToPlayer.erase(player.id);

    printf("Client Id: %d, tcpFd: %d disconnected\n", player.id, player.connection.tcpFd);
    std::string disconnectMessage = "D;" + std::to_string(player.id) + "~";

    connectionService.sendTcpMessageToAll(disconnectMessage);
}

void sendHeartbeat() {
    // TODO
}

int main(int argc, char **argv) {
    if (argc != 3) 
        error(1, 0, "Usage: %s <tcp-port> <udp-port>", argv[0]);

    int tcpPort = atoi(argv[1]);
    int udpPort = atoi(argv[2]);

    if (tcpPort == udpPort)
        error(1, 0, "TCP and UDP ports must differ");
    if (tcpPort > 65535 || tcpPort < 1024 || udpPort > 65535 || udpPort < 1024)
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
