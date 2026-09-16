#include "network.h"

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

bool NetworkUtils::isPortAvailable(uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) return false;
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    bool available = (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    
    return available;
}

std::string NetworkUtils::getLocalIP() {
    // 简单实现，返回 localhost
    return "127.0.0.1";
}

bool NetworkUtils::checkConnectivity(const std::string &host, uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) return false;
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host.c_str());
    
    bool connected = (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    
    return connected;
}
