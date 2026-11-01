#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include <iostream>

#include "connectionService.hpp"
#include "utils.hpp"

Connection::Connection(
    int tcpFd,
    void(*onNewMessage)(const Connection& connection, const std::string& message),
    sockaddr_in udpAddress): tcpFd(tcpFd), onNewMessage(onNewMessage), udpAddress(udpAddress) {}

ConnectionService::ConnectionService(
    int tcpPort,
    void(*onNewConnection)(Connection& connection), 
    void(*onNewTcpMessage)(const Connection& connection, const std::string& message),
    void(*onPlayerDisconnected)(const Connection& connection),
    
    bool createUdpSocket,
    void(*onNewUdpMessage)(char* messageBuffer)
): onNewConnection(onNewConnection), onNewTcpMessage(onNewTcpMessage), onPlayerDisconnected(onPlayerDisconnected), onNewUdpMessage(onNewUdpMessage) 
{

    tcpFd = getaddrinfo_socket_bind_listen(SOCK_STREAM, tcpPort, this->tcpPort);
    setNonBlocking(tcpFd);
    int flag = 1;
    setsockopt(tcpFd, IPPROTO_TCP, O_NDELAY, (char *)&flag, sizeof(int));

    if(createUdpSocket){
        udpFd = getaddrinfo_socket_bind_listen(SOCK_DGRAM, RANDOM_PORT, this->udpPort);
        setNonBlocking(udpFd);
    
        printf("TCP: %d, UDP: %d\n", this->tcpPort, udpPort);
    }
    else{
        printf("TCP: %d\n", this->tcpPort);
    }

    epollDesc = epoll_create1(0);
    std::cout << "Epoll created, desc: " << epollDesc << std::endl;

    ee.events = EPOLLIN;
    ee.data.fd = STDOUT_FILENO;
    epoll_ctl(epollDesc, EPOLL_CTL_ADD, STDIN_FILENO, &ee);
    ee.data.u32 = -1;
    epoll_ctl(epollDesc, EPOLL_CTL_ADD, tcpFd, &ee);

    if(createUdpSocket){
        ee.data.u32 = -2;
        epoll_ctl(epollDesc, EPOLL_CTL_ADD, udpFd, &ee);
    }
}

void ConnectionService::listenForEvents(int timeout)
{
    if(epoll_wait(epollDesc, &ee, 1, timeout) <= 0){
        return;
    }

    if(ee.data.u32 == (uint32_t)-1){
        handleNewClient(ee);          
        return;
    }
    
    if(ee.data.u32 == (uint32_t)-2){
        handleUdpMessages();          
        return;
    }

    handleTcpMessage(ee);
}

void ConnectionService::handleNewClient(epoll_event& ee){
    sockaddr_storage clientAddr{};
    socklen_t clientAddrSize = sizeof(clientAddr);

    int clientFd = accept(tcpFd, (sockaddr *)&clientAddr, &clientAddrSize);
    if (clientFd == -1){
        perror("accept failed");
        return;
    }

    setNonBlocking(clientFd);
    int flag = 1;
    setsockopt(clientFd, IPPROTO_TCP, O_NDELAY, (char *)&flag, sizeof(int));

    char host[NI_MAXHOST], port[NI_MAXSERV];
    getnameinfo((sockaddr *)&clientAddr, clientAddrSize, host, NI_MAXHOST, port, NI_MAXSERV, 0);
    printf("New connection from: %s:%s (fd: %d)\n", host, port, clientFd);

    createNewConnection(clientFd, onNewTcpMessage, *(sockaddr_in*)&clientAddr);

    if(onNewConnection != nullptr)
        onNewConnection(fdToConnection[clientFd]);
}

void ConnectionService::handleTcpMessage(epoll_event& ee) {
    int connectionFd = ee.data.fd;

    if(!fdToConnection.contains(connectionFd)){
        printf("[ConnectionService] Received message from unknown connection: %d\n", connectionFd);
        closeConnection(connectionFd);
        return;
    }

    Connection& connection = fdToConnection[connectionFd];

    char buffer[255];
    int count = read(connection.tcpFd, buffer, 255);

    if (count < 1) {
        if (count == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return;
        }

        handleDisconnect(connection);
        return;
    }

    connection.inputBuffer += std::string(buffer, count);

    size_t pos = 0;
    while ((pos = connection.inputBuffer.find('~')) != std::string::npos) {
        std::string message = connection.inputBuffer.substr(0, pos);
        connection.inputBuffer.erase(0, pos + 1);

        if(connection.onNewMessage != nullptr){
            connection.onNewMessage(connection, message);
        }
    }
}

int ConnectionService::getaddrinfo_socket_bind_listen(int sockType, int port, int& resultPort) {
    addrinfo *res, hints{};
    hints.ai_flags = AI_PASSIVE;
    // hints.ai_socktype = SOCK_STREAM;
    hints.ai_socktype = sockType;

    int rv;

    if(port == RANDOM_PORT)
        rv = getaddrinfo(nullptr, "0", &hints, &res);
    else
        rv = getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &res);

    if (rv) 
        error(1, 0, "getaddrinfo: %s", gai_strerror(rv));

    // create socket
    int sockFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockFd == -1) error(1, errno, "socket failed");

    // try to set REUSEADDR
    const int one = 1;
    setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    // bind to address as resolved by getaddrinfo
    if (bind(sockFd, res->ai_addr, res->ai_addrlen))
        error(1, errno, "bind failed");

    // enter listening mode
    if(sockType == SOCK_STREAM){
        if (listen(sockFd, 1))
            error(1, errno, "listen failed");
    }

    sockaddr_storage bound{};
    socklen_t boundLen = sizeof(bound);
    if (getsockname(sockFd, (sockaddr*)&bound, &boundLen))
        error(1, errno, "getsockname failed");

    char result_host[NI_MAXHOST], result_port[NI_MAXSERV];
    rv = getnameinfo((sockaddr*)&bound, boundLen,
                     result_host, NI_MAXHOST,
                     result_port, NI_MAXSERV,
                     NI_NUMERICHOST | NI_NUMERICSERV);
    if (rv)
        error(1, 0, "getnameinfo: %s", gai_strerror(rv));

    if(port != RANDOM_PORT && port != atoi(result_port)){
        printf("Failed creating socket at port: %s\n", std::to_string(port).c_str());
        exit(1);
    }

    resultPort = atoi(result_port);

    printf("Created socket: %s:%s (fd: %d)\n", result_host, result_port, sockFd);
    
    freeaddrinfo(res);
    return sockFd;
}

void ConnectionService::setNonBlocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

void ConnectionService::closeConnection(int connectionFd){
    epoll_ctl(epollDesc, EPOLL_CTL_DEL, connectionFd, nullptr);
    shutdown(connectionFd, SHUT_RDWR);
    close(connectionFd);
}

void ConnectionService::handleDisconnect(const Connection& connection){
    fdToConnection.erase(connection.tcpFd);
    closeConnection(connection.tcpFd);

    if(onPlayerDisconnected != nullptr){
        onPlayerDisconnected(connection);
    }
}

int ConnectionService::getUdpPort() const {
    return udpPort;
}

int ConnectionService::getTcpPort() const {
    return tcpPort;
}

void ConnectionService::closeAllConnections() {
    for (const auto& [fd, connection] : fdToConnection) {
        closeConnection(fd);
    }
    fdToConnection.clear();

    close(tcpFd);
    close(udpFd);
}

void ConnectionService::sendTcpMessageTo(const Connection& connection, const std::string& message){
    if (message.size() != send(connection.tcpFd, message.c_str(), message.size(), MSG_DONTWAIT))
    {
        printf("Write failed on %d\n", connection.tcpFd);
        handleDisconnect(connection);
    }
}

void ConnectionService::sendTcpMessageToAllExcept(const Connection& connectionToSkip, const std::string& message){
    for(auto [fd, connection] : fdToConnection){
        if(connection == connectionToSkip){
            continue;
        }
        sendTcpMessageTo(connection, message);
    }
}

void ConnectionService::sendTcpMessageToAll(const std::string& message){
    for(auto [fd, connection] : fdToConnection)
        sendTcpMessageTo(connection, message);
}

void ConnectionService::sendUdpMessageTo(const Connection& connection, const std::string& message){
    if(!connection.isUdpInitialized){
        return;
    }

    if (message.size() != sendto(udpFd, message.c_str(), message.size(), 0, (sockaddr *)&connection.udpAddress, sizeof(connection.udpAddress)))
    {
        printf("Write failed on UDP socket\n");
    }
}
        
void ConnectionService::sendUdpMessageToAll(const std::string& message){
    for(auto [fd, connection] : fdToConnection)
        sendUdpMessageTo(connection, message);
}

void ConnectionService::handleUdpMessages(){
    char buffer[255];
    
    while(true) {
        int count = recvfrom(udpFd, buffer, 255, MSG_DONTWAIT, nullptr, nullptr);
        
        if (count < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                //TODO: Maybe we should hadle this;
                break;
            }
            break;
        }
        
        if(onNewUdpMessage != nullptr){
            onNewUdpMessage(buffer);
        }
    }
}

Connection& ConnectionService::createNewConnection(int fd, void(*onNewMessage)(const Connection& connection, const std::string& message), sockaddr_in udpAddress){
    epoll_event ee{};
    ee.events = EPOLLIN;
    ee.data.fd = fd;
    epoll_ctl(epollDesc, EPOLL_CTL_ADD, fd, &ee);

    fdToConnection.try_emplace(fd, fd, onNewMessage, udpAddress);

    return fdToConnection[fd];
}