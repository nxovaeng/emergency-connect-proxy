#include "openvpn_tunnel.h"
#include "utils/logger.h"
#include "utils/process.h"
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <signal.h>
#endif

OpenVPNTunnel::OpenVPNTunnel() {}

OpenVPNTunnel::~OpenVPNTunnel() {
    disconnect();
}

bool OpenVPNTunnel::initialize(const std::string &configPath, const std::string &username, const std::string &password) {
    configPath_ = configPath;
    username_ = username;
    password_ = password;
    return true;
}

bool OpenVPNTunnel::connect(const std::string &ip, uint16_t port, const std::string &protocol) {
    if (connected_) {
        return false;
    }
    
    std::string configFile = generateTempConfigFile(ip, port, protocol);
    
    if (!startOpenVPNProcess(configFile)) {
        return false;
    }
    
    connected_ = true;
    if (statusCallback_) {
        statusCallback_("Connected to " + ip + ":" + std::to_string(port));
    }
    
    return true;
}

bool OpenVPNTunnel::disconnect() {
    if (!connected_) {
        return true;
    }
    
    if (!stopOpenVPNProcess()) {
        return false;
    }
    
    connected_ = false;
    if (statusCallback_) {
        statusCallback_("Disconnected");
    }
    
    return true;
}

bool OpenVPNTunnel::isConnected() const {
    return connected_;
}

std::string OpenVPNTunnel::getTunnelInterface() const {
    // 实现获取隧道接口名称
    return "tap0";  // 示例
}

std::string OpenVPNTunnel::getTunnelIP() const {
    // 实现获取隧道 IP
    return "10.8.0.1";  // 示例
}

std::vector<std::string> OpenVPNTunnel::getDnsServers() const {
    return {"8.8.8.8", "8.8.4.4"};
}

void OpenVPNTunnel::setStatusCallback(StatusCallback callback) {
    statusCallback_ = callback;
}

std::string OpenVPNTunnel::generateTempConfigFile(const std::string &ip, uint16_t port, const std::string &protocol) {
    // 基于原始 OVPN 配置和端点信息生成临时配置
    // 实现配置文件生成逻辑
    return "/tmp/emergency_ovpn_temp.conf";
}

bool OpenVPNTunnel::startOpenVPNProcess(const std::string &configFile) {
    // 启动 OpenVPN 进程
    ProcessManager pm;
    return pm.startProcess("openvpn", {"--config", configFile, "--auth-user-pass"}, processPid_);
}

bool OpenVPNTunnel::stopOpenVPNProcess() {
    if (processPid_ == -1) {
        return false;
    }
    
    ProcessManager pm;
    
#ifdef _WIN32
    return TerminateProcess(OpenProcess(PROCESS_TERMINATE, FALSE, processPid_), 0);
#else
    return kill(processPid_, SIGTERM) == 0;
#endif
}
