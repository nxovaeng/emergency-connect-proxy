#include "proxy_server.h"
#include <iostream>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

ProxyServer::ProxyServer() : port_(0), serverSocket_(-1) {}

ProxyServer::~ProxyServer() {
    stop();
}

bool ProxyServer::start(const std::string &bindAddress, uint16_t port) {
    bindAddress_ = bindAddress;
    port_ = port;
    
    if (!setupServer()) {
        return false;
    }
    
    running_ = true;
    acceptThread_ = std::thread(&ProxyServer::acceptConnections, this);
    
    return true;
}

bool ProxyServer::stop() {
    running_ = false;
    
    if (serverSocket_ != -1) {
#ifdef _WIN32
        closesocket(serverSocket_);
#else
        close(serverSocket_);
#endif
        serverSocket_ = -1;
    }
    
    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
    
    return true;
}

bool ProxyServer::isRunning() const {
    return running_;
}

std::string ProxyServer::getAddress() const {
    return bindAddress_;
}

uint16_t ProxyServer::getPort() const {
    return port_;
}

void ProxyServer::setTunnelInterface(const std::string &iface) {
    tunnelInterface_ = iface;
}

void ProxyServer::setEnableSocks5(bool enable) {
    enableSocks5_ = enable;
}

void ProxyServer::setEnableHttp(bool enable) {
    enableHttp_ = enable;
}

bool ProxyServer::setupServer() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
#endif
    
    serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket_ < 0) {
        return false;
    }
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    serverAddr.sin_addr.s_addr = inet_addr(bindAddress_.c_str());
    
    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        return false;
    }
    
    if (listen(serverSocket_, 128) < 0) {
        return false;
    }
    
    return true;
}

void ProxyServer::acceptConnections() {
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientAddrLen);
        
        if (clientSocket < 0) {
            if (running_) {
                continue;
            } else {
                break;
            }
        }
        
        // 处理客户端连接
        handleClientConnection(clientSocket);
    }
}

void ProxyServer::handleClientConnection(int clientSocket) {
    // 简单的 HTTP 代理实现
    char buffer[4096];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    
    if (bytesRead > 0) {
        // 解析 HTTP 请求并转发到真实服务器
        // 通过 OpenVPN 隧道
    }
    
#ifdef _WIN32
    closesocket(clientSocket);
#else
    close(clientSocket);
#endif
}
