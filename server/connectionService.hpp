#pragma once

#include <string>
#include <netdb.h>
#include <sys/epoll.h>
#include <unordered_map>

#define NO_UDP_PORT -1
#define RANDOM_UDP_PORT -2

class Connection{
    public:
        int tcpFd;
        std::string inputBuffer;
        
        sockaddr_in udpAddress{};
        bool isUdpInitialized = false;

        bool operator==(const Connection& other) const {
            return tcpFd == other.tcpFd;
        }
};

namespace std {
    template <>
    struct hash<Connection> {
        size_t operator()(const Connection& connection) const noexcept {
            return std::hash<int>{}(connection.tcpFd);
        }
    };
}

class ConnectionService {
    public:
        ConnectionService() = default;

        ConnectionService(
            const char *tcpPort,
            void(*onNewConnection)(Connection& connection), 
            void(*onNewTcpMessage)(const Connection& connection, const std::string& message),
            void(*onPlayerDisconnected)(const Connection& connection),
            
            const char *udpPort, 
            void(*onNewUdpMessage)(char* messageBuffer)
        );

        void listenForEvents(int timeout);  
        
        void sendTcpMessageTo(const Connection& connection, const std::string& message);
        void sendTcpMessageToAllExcept(const Connection& connectionToSkip, const std::string& message);
        void sendTcpMessageToAll(const std::string& message);

        void sendUdpMessageTo(const Connection& connection, const std::string& message);
        void sendUdpMessageToAll(const std::string& message);
        int getUdpPort() const;

        void closeAllConnections();

    private:
        
        int tcpFd;
        int udpFd;
        int udpPort;
        int epollDesc;
        epoll_event ee;
        std::unordered_map<int, Connection> fdToConnection;

        void(*onNewConnection)(Connection& connection);
        void(*onNewTcpMessage)(const Connection& connection, const std::string& message);
        void(*onPlayerDisconnected)(const Connection& connection);
    
        void(*onNewUdpMessage)(char* messageBuffer);

        int getaddrinfo_socket_bind_listen(int sockType, const char *port);
        void setNonBlocking(int sock);
        void handleNewClient(epoll_event& ee);
        void handleTcpMessage(epoll_event& ee);
        void handleUdpMessages();
        void closeConnection(int connectionFd);
        void handleDisconnect(const Connection& connection);
};