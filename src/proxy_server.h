#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <cstdint>

class ProxyServer {
public:
    ProxyServer();
    ~ProxyServer();
    
    // 启动代理
    bool start(const std::string &bindAddress, uint16_t port);
    
    // 停止代理
    bool stop();
    
    // 状态查询
    bool isRunning() const;
    std::string getAddress() const;
    uint16_t getPort() const;
    
    // 配置
    void setTunnelInterface(const std::string &iface);
    void setEnableSocks5(bool enable);
    void setEnableHttp(bool enable);
    
private:
    std::string bindAddress_;
    uint16_t port_;
    std::string tunnelInterface_;
    bool enableSocks5_ = true;
    bool enableHttp_ = true;
    std::atomic<bool> running_{false};
    
    int serverSocket_ = -1;
    std::thread acceptThread_;
    
    bool setupServer();
    void acceptConnections();
    void handleClientConnection(int clientSocket);
    int connectToRemote(const std::string &host, uint16_t port);
    void pipeSockets(int sock1, int sock2);
    void closeSocket(int sock);
};
