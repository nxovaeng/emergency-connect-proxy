#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

class OpenVPNTunnel {
public:
    OpenVPNTunnel();
    ~OpenVPNTunnel();
    
    // 初始化
    bool initialize(const std::string &configPath, const std::string &username, const std::string &password);
    
    // 连接
    bool connect(const std::string &ip, uint16_t port, const std::string &protocol);
    
    // 断开
    bool disconnect();
    
    // 状态查询
    bool isConnected() const;
    std::string getTunnelInterface() const;
    std::string getTunnelIP() const;
    std::vector<std::string> getDnsServers() const;
    
    // 状态回调
    using StatusCallback = std::function<void(const std::string &status)>;
    void setStatusCallback(StatusCallback callback);
    
private:
    std::string configPath_;
    std::string username_;
    std::string password_;
    bool connected_ = false;
    int processPid_ = -1;
    
    StatusCallback statusCallback_;
    
    std::string generateTempConfigFile(const std::string &ip, uint16_t port, const std::string &protocol);
    bool startOpenVPNProcess(const std::string &configFile);
    bool stopOpenVPNProcess();
};
