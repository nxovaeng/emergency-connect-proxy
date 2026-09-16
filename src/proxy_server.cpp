#include "proxy_server.h"
#include "utils/logger.h"
#include <iostream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <sys/select.h>
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

void ProxyServer::closeSocket(int sock) {
    if (sock >= 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }
}

bool ProxyServer::stop() {
    running_ = false;
    
    if (serverSocket_ != -1) {
        closeSocket(serverSocket_);
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

    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    serverAddr.sin_addr.s_addr = inet_addr(bindAddress_.c_str());
    
    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        closeSocket(serverSocket_);
        serverSocket_ = -1;
        return false;
    }
    
    if (listen(serverSocket_, 128) < 0) {
        closeSocket(serverSocket_);
        serverSocket_ = -1;
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
        
        // 异步并发处理客户端连接
        std::thread(&ProxyServer::handleClientConnection, this, clientSocket).detach();
    }
}

int ProxyServer::connectToRemote(const std::string &host, uint16_t port) {
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *result = nullptr;
    std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0 || result == nullptr) {
        return -1;
    }

    int remoteSock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (remoteSock < 0) {
        freeaddrinfo(result);
        return -1;
    }

#ifdef _WIN32
    DWORD timeoutMs = 6000;
    setsockopt(remoteSock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
    setsockopt(remoteSock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
#else
    struct timeval tv;
    tv.tv_sec = 6;
    tv.tv_usec = 0;
    setsockopt(remoteSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(remoteSock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif

    if (connect(remoteSock, result->ai_addr, result->ai_addrlen) < 0) {
        closeSocket(remoteSock);
        freeaddrinfo(result);
        return -1;
    }

    freeaddrinfo(result);
    return remoteSock;
}

void ProxyServer::pipeSockets(int sock1, int sock2) {
    char buf[8192];
    int maxFd = std::max(sock1, sock2) + 1;

    while (running_) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock1, &fds);
        FD_SET(sock2, &fds);

        struct timeval tv;
        tv.tv_sec = 30;
        tv.tv_usec = 0;

        int ret = select(maxFd, &fds, nullptr, nullptr, &tv);
        if (ret <= 0) {
            break;
        }

        if (FD_ISSET(sock1, &fds)) {
            int n = recv(sock1, buf, sizeof(buf), 0);
            if (n <= 0) break;
            int sent = send(sock2, buf, n, 0);
            if (sent <= 0) break;
        }
        if (FD_ISSET(sock2, &fds)) {
            int n = recv(sock2, buf, sizeof(buf), 0);
            if (n <= 0) break;
            int sent = send(sock1, buf, n, 0);
            if (sent <= 0) break;
        }
    }
}

void ProxyServer::handleClientConnection(int clientSocket) {
    char buffer[4096];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead <= 0) {
        closeSocket(clientSocket);
        return;
    }
    buffer[bytesRead] = '\0';
    std::string request(buffer, bytesRead);

    // 1. 处理 HTTP CONNECT 代理隧道 (用于 HTTPS/SSL 流量，如 Windscribe 客户端的 API 登录)
    if (enableHttp_ && request.rfind("CONNECT ", 0) == 0) {
        size_t hostStart = 8;
        size_t hostEnd = request.find(' ', hostStart);
        if (hostEnd != std::string::npos) {
            std::string hostPort = request.substr(hostStart, hostEnd - hostStart);
            size_t colon = hostPort.find(':');
            std::string host = (colon != std::string::npos) ? hostPort.substr(0, colon) : hostPort;
            uint16_t port = (colon != std::string::npos) ? static_cast<uint16_t>(std::stoi(hostPort.substr(colon + 1))) : 443;

            int remoteSocket = connectToRemote(host, port);
            if (remoteSocket >= 0) {
                const char *okResp = "HTTP/1.1 200 Connection Established\r\n\r\n";
                send(clientSocket, okResp, std::strlen(okResp), 0);
                pipeSockets(clientSocket, remoteSocket);
                closeSocket(remoteSocket);
            } else {
                const char *errResp = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
                send(clientSocket, errResp, std::strlen(errResp), 0);
            }
        }
    }
    // 2. 处理 SOCKS5 握手协议 (首字节为 0x05)
    else if (enableSocks5_ && static_cast<uint8_t>(buffer[0]) == 0x05) {
        char authResp[2] = {0x05, 0x00}; // 无需认证
        send(clientSocket, authResp, 2, 0);

        int reqLen = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (reqLen >= 7 && buffer[0] == 0x05 && buffer[1] == 0x01) { // CONNECT 命令
            std::string targetHost;
            uint16_t targetPort = 0;

            if (buffer[3] == 0x01) { // IPv4 地址
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &buffer[4], ipStr, sizeof(ipStr));
                targetHost = ipStr;
                targetPort = ntohs(*reinterpret_cast<uint16_t*>(&buffer[8]));
            } else if (buffer[3] == 0x03) { // 域名
                uint8_t domainLen = static_cast<uint8_t>(buffer[4]);
                targetHost = std::string(&buffer[5], domainLen);
                targetPort = ntohs(*reinterpret_cast<uint16_t*>(&buffer[5 + domainLen]));
            }

            if (!targetHost.empty() && targetPort > 0) {
                int remoteSocket = connectToRemote(targetHost, targetPort);
                if (remoteSocket >= 0) {
                    char okResp[10] = {0x05, 0x00, 0x00, 0x01, 0, 0, 0, 0, 0, 0};
                    send(clientSocket, okResp, 10, 0);
                    pipeSockets(clientSocket, remoteSocket);
                    closeSocket(remoteSocket);
                } else {
                    char errResp[10] = {0x05, 0x05, 0x00, 0x01, 0, 0, 0, 0, 0, 0};
                    send(clientSocket, errResp, 10, 0);
                }
            }
        }
    }
    // 3. 普通 HTTP GET/POST 转发
    else if (enableHttp_) {
        size_t hostPos = request.find("Host: ");
        if (hostPos != std::string::npos) {
            size_t hostStart = hostPos + 6;
            size_t hostEnd = request.find("\r\n", hostStart);
            if (hostEnd != std::string::npos) {
                std::string hostPort = request.substr(hostStart, hostEnd - hostStart);
                size_t colon = hostPort.find(':');
                std::string host = (colon != std::string::npos) ? hostPort.substr(0, colon) : hostPort;
                uint16_t port = (colon != std::string::npos) ? static_cast<uint16_t>(std::stoi(hostPort.substr(colon + 1))) : 80;

                int remoteSocket = connectToRemote(host, port);
                if (remoteSocket >= 0) {
                    send(remoteSocket, buffer, bytesRead, 0);
                    pipeSockets(clientSocket, remoteSocket);
                    closeSocket(remoteSocket);
                }
            }
        }
    }
    
    closeSocket(clientSocket);
}
