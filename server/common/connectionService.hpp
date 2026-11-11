#pragma once

#include <string>
#include <netdb.h>
#include <sys/epoll.h>
#include <unordered_map>

#define RANDOM_PORT 0

class Connection{
    public:
        int tcpFd;
        std::string inputBuffer;
        void(*onNewMessage)(const Connection& connection, const std::string& message);
        
        sockaddr_in udpAddress;
        bool isUdpInitialized = false;

        Connection() = default;

        Connection(int tcpFd, void(*onNewMessage)(const Connection& connection, const std::string& message), sockaddr_in udpAddress);

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
            int tcpPort,
            void(*onNewConnection)(Connection& connection), 
            void(*onNewTcpMessage)(const Connection& connection, const std::string& message),
            void(*onPlayerDisconnected)(const Connection& connection),
            
            bool createUdpSocket, 
            void(*onNewUdpMessage)(char* messageBuffer)
        );

        void listenForEvents(int timeout);

        int getUdpPort() const;
        int getTcpPort() const;

        void closeAllConnections();

        Connection& createConnection(int fd, void(*onNewMessage)(const Connection& connection, const std::string& message), sockaddr_in udpAddress, bool isAdditional);

        template <typename MessageType>
        void sendTcpMessageTo(const Connection& connection, MessageType& message){
            const std::string builtMessage = message.Build() + messageEndChar;
            sendTcpMessageTo(connection, builtMessage);
        }

        template <typename MessageType>
        void sendTcpMessageToAllExcept(const Connection& connectionToSkip, MessageType& message){
            const std::string builtMessage = message.Build() + messageEndChar;

            for(auto [fd, connection] : fdToConnection){
                if(connection == connectionToSkip){
                    continue;
                }

                sendTcpMessageTo(connection, builtMessage);
            }
        }

        template <typename MessageType>
        void sendTcpMessageToAll(MessageType& message){
            const std::string builtMessage = message.Build() + messageEndChar;

            for(auto [fd, connection] : fdToConnection)
                sendTcpMessageTo(connection, builtMessage);
        }

        template <typename MessageType>
        void sendUdpMessageTo(const Connection& connection, MessageType& message){
            const std::string builtMessage = message.Build() + messageEndChar;
            sendUdpMessageTo(connection, builtMessage);
        }

        template <typename MessageType>
        void sendUdpMessageToAll(MessageType& message){
            const std::string builtMessage = message.Build() + messageEndChar;

            for(auto [fd, connection] : fdToConnection)
                sendUdpMessageTo(connection, builtMessage);
        }

    private:
        static const char messageEndChar = '~';
        
        int tcpFd = -1;
        int udpFd = -1;
        int tcpPort = -1;
        int udpPort = -1;
        int epollDesc = -1;
        epoll_event ee;
        std::unordered_map<int, Connection> fdToConnection;
        std::unordered_map<int, Connection> additionalFdToConnection;

        void(*onNewConnection)(Connection& connection);
        void(*onNewTcpMessage)(const Connection& connection, const std::string& message);
        void(*onPlayerDisconnected)(const Connection& connection);
    
        void(*onNewUdpMessage)(char* messageBuffer);

        int getaddrinfo_socket_bind_listen(int sockType, int port, int& resultPort);
        void setNonBlocking(int sock);
        void handleNewClient(epoll_event& ee);
        void handleTcpMessage(epoll_event& ee);
        void handleUdpMessages();
        void closeConnection(int connectionFd);
        void handleDisconnect(const Connection& connection);

        void sendTcpMessageTo(const Connection& connection, const std::string& message);
        void sendUdpMessageTo(const Connection& connection, const std::string& message);
};