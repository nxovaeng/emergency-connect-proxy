#pragma once

#include <string>
#include <vector>

class NetworkUtils {
public:
    // 检查端口是否可用
    static bool isPortAvailable(uint16_t port);
    
    // 获取本地 IP
    static std::string getLocalIP();
    
    // 检查网络连接
    static bool checkConnectivity(const std::string &host, uint16_t port);
};
