#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include "wsnet/WSNet.h"

struct EmergencyEndpoint {
    std::string ip;
    uint16_t port;
    std::string protocol;  // "udp" or "tcp"
};

struct ProxyConfig {
    // 代理配置
    uint16_t proxyPort = 8888;
    std::string bindAddress = "127.0.0.1";
    bool enableSocks5 = true;
    bool enableHttp = true;
    
    // OpenVPN 配置
    std::string ovpnConfigPath;
    std::string username;
    std::string password;
    int openvpnTimeout = 30;
    int openvpnRetries = 3;
    
    // 端点配置
    bool autoFetchEndpoints = true;
    std::vector<EmergencyEndpoint> endpoints;
    
    // 日志配置
    std::string logLevel = "info";
    std::string logFile;
};

class EmergencyConnectProxy {
public:
    EmergencyConnectProxy();
    ~EmergencyConnectProxy();
    
    // 初始化配置
    bool initialize(const ProxyConfig &config);
    
    // 自动获取远程端点信息 (模拟 Windscribe Emergency Connection Attempt Strategy)
    bool fetchRemoteEndpoints(bool async = false, std::function<void(bool success)> onComplete = nullptr);
    
    // 获取当前配置的端点列表
    const std::vector<EmergencyEndpoint>& getEndpoints() const;
    
    // 启动代理
    bool start();
    
    // 停止代理
    bool stop();
    
    // 获取状态
    bool isRunning() const;
    std::string getStatus() const;
    
    // 获取代理 URL
    std::string getProxyUrl() const;
    
    // 日志回调
    using LogCallback = std::function<void(const std::string &level, const std::string &message)>;
    void setLogCallback(LogCallback callback);
    
private:
    ProxyConfig config_;
    std::atomic<bool> running_{false};
    
    class OpenVPNTunnel* vpnTunnel_ = nullptr;
    class ProxyServer* proxyServer_ = nullptr;
    
    LogCallback logCallback_;
    std::thread proxyThread_;
    
    void log(const std::string &level, const std::string &message);
    
    bool setupVPN();
    bool setupProxy();
    bool cleanup();
};
