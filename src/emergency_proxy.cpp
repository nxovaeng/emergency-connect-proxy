#include "emergency_proxy.h"
#include "openvpn_tunnel.h"
#include "proxy_server.h"
#include "system_proxy.h"
#include "utils/logger.h"
#include <iostream>

EmergencyConnectProxy::EmergencyConnectProxy()
    : vpnTunnel_(nullptr), proxyServer_(nullptr), sysProxy_(nullptr) {}

EmergencyConnectProxy::~EmergencyConnectProxy() {
    stop();
}

bool EmergencyConnectProxy::initialize(const ProxyConfig &config) {
    config_ = config;
    
    if (!config_.endpoints.empty()) {
        log("info", "Loaded " + std::to_string(config_.endpoints.size()) + " endpoints");
    }
    
    return true;
}

bool EmergencyConnectProxy::start() {
    if (running_) {
        log("warn", "Proxy is already running");
        return false;
    }
    
    log("info", "Starting Emergency Connect Proxy...");
    
    // 设置 VPN
    if (!setupVPN()) {
        log("error", "Failed to setup VPN tunnel");
        return false;
    }
    
    // 设置代理服务器
    if (!setupProxy()) {
        log("error", "Failed to setup proxy server");
        cleanup();
        return false;
    }
    
    running_ = true;
    log("info", "Proxy started successfully on " + getProxyUrl());
    
    return true;
}

bool EmergencyConnectProxy::stop() {
    if (!running_) {
        return true;
    }
    
    log("info", "Stopping proxy...");
    running_ = false;
    
    return cleanup();
}

bool EmergencyConnectProxy::isRunning() const {
    return running_;
}

std::string EmergencyConnectProxy::getStatus() const {
    if (running_) {
        return "Running on " + getProxyUrl();
    }
    return "Stopped";
}

std::string EmergencyConnectProxy::getProxyUrl() const {
    return "http://" + config_.bindAddress + ":" + std::to_string(config_.proxyPort);
}

void EmergencyConnectProxy::setLogCallback(LogCallback callback) {
    logCallback_ = callback;
}

void EmergencyConnectProxy::log(const std::string &level, const std::string &message) {
    if (logCallback_) {
        logCallback_(level, message);
    } else {
        std::cout << "[" << level << "] " << message << std::endl;
    }
}

bool EmergencyConnectProxy::setupVPN() {
    vpnTunnel_ = new OpenVPNTunnel();
    return true;
}

bool EmergencyConnectProxy::setupProxy() {
    proxyServer_ = new ProxyServer();
    
    if (!proxyServer_->start(config_.bindAddress, config_.proxyPort)) {
        log("error", "Failed to start proxy server on port " + std::to_string(config_.proxyPort));
        return false;
    }
    
    return true;
}

bool EmergencyConnectProxy::setupSystemProxy() {
    sysProxy_ = new SystemProxy();
    return sysProxy_->setProxy(getProxyUrl());
}

bool EmergencyConnectProxy::cleanup() {
    if (proxyServer_) {
        proxyServer_->stop();
        delete proxyServer_;
        proxyServer_ = nullptr;
    }
    
    if (vpnTunnel_) {
        delete vpnTunnel_;
        vpnTunnel_ = nullptr;
    }
    
    if (sysProxy_) {
        sysProxy_->restore();
        delete sysProxy_;
        sysProxy_ = nullptr;
    }
    
    return true;
}
