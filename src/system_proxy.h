#pragma once

#include <string>

class SystemProxy {
public:
    SystemProxy();
    ~SystemProxy();
    
    // 设置系统代理
    bool setProxy(const std::string &proxyUrl);
    
    // 恢复原代理
    bool restore();
    
    // 获取当前系统代理
    std::string getCurrentProxy() const;
    
private:
    std::string originalProxySettings_;
    
#ifdef _WIN32
    bool setWindowsProxy(const std::string &proxyUrl);
    bool restoreWindowsProxy();
#elif __APPLE__
    bool setMacProxy(const std::string &proxyUrl);
    bool restoreMacProxy();
#else
    bool setLinuxProxy(const std::string &proxyUrl);
    bool restoreLinuxProxy();
#endif
};
