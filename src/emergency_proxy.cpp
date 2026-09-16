#include "emergency_proxy.h"
#include "openvpn_tunnel.h"
#include "proxy_server.h"
#include "utils/logger.h"
#include <iostream>

EmergencyConnectProxy::EmergencyConnectProxy()
    : vpnTunnel_(nullptr), proxyServer_(nullptr) {}

EmergencyConnectProxy::~EmergencyConnectProxy() {
    stop();
}

bool EmergencyConnectProxy::initialize(const ProxyConfig &config) {
    config_ = config;
    
    // 填充默认 Windscribe 应急认证信息
    if (config_.username.empty()) {
        config_.username = wsnet::emergencyConnect()->username();
    }
    if (config_.password.empty()) {
        config_.password = wsnet::emergencyConnect()->password();
    }
    
    // 如果启用了自动获取远端端点，或者本地尚未配置任何端点
    if (config_.autoFetchEndpoints || config_.endpoints.empty()) {
        fetchRemoteEndpoints(false);
    } else {
        log("info", "Loaded " + std::to_string(config_.endpoints.size()) + " static endpoints from configuration");
    }
    
    return true;
}

bool EmergencyConnectProxy::fetchRemoteEndpoints(bool async, std::function<void(bool success)> onComplete) {
    log("info", "Fetching remote emergency endpoints via wsnet::emergencyConnect()...");

    auto processEndpoints = [this](const std::vector<std::shared_ptr<wsnet::WSNetEmergencyConnectEndpoint>> &endpoints) {
        std::vector<EmergencyEndpoint> plainEndpoints;
        for (const auto &ep : endpoints) {
            EmergencyEndpoint e;
            e.ip = ep->ip();
            e.port = ep->port();
            e.protocol = (ep->protocol() == wsnet::Protocol::kTcp) ? "tcp" : "udp";
            plainEndpoints.push_back(e);
        }

        // 保留原有的自定义端点
        for (const auto &orig : config_.endpoints) {
            bool found = false;
            for (const auto &p : plainEndpoints) {
                if (p.ip == orig.ip && p.port == orig.port && p.protocol == orig.protocol) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                plainEndpoints.push_back(orig);
            }
        }

        config_.endpoints = std::move(plainEndpoints);
        log("info", "Active emergency endpoints count: " + std::to_string(config_.endpoints.size()));
    };

    if (async) {
        wsnet::emergencyConnect()->getIpEndpoints([processEndpoints, onComplete](const std::vector<std::shared_ptr<wsnet::WSNetEmergencyConnectEndpoint>> &endpoints) {
            processEndpoints(endpoints);
            if (onComplete) onComplete(true);
        });
        return true;
    } else {
        auto endpoints = wsnet::emergencyConnect()->getIpEndpointsSync(4000);
        if (!endpoints.empty()) {
            processEndpoints(endpoints);
            if (onComplete) onComplete(true);
            return true;
        } else {
            log("warn", "Failed to fetch remote endpoints via wsnet");
            if (onComplete) onComplete(false);
            return false;
        }
    }
}

const std::vector<EmergencyEndpoint>& EmergencyConnectProxy::getEndpoints() const {
    return config_.endpoints;
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
    vpnTunnel_->initialize(config_.ovpnConfigPath, config_.username, config_.password);
    
    vpnTunnel_->setStatusCallback([this](const std::string &status) {
        log("info", "[VPN] " + status);
    });

    if (config_.endpoints.empty()) {
        log("warn", "No emergency endpoints available, attempting to fetch now...");
        fetchRemoteEndpoints(false);
    }

    if (config_.endpoints.empty()) {
        log("error", "Failed to obtain any emergency endpoints for VPN tunnel");
        return false;
    }

    log("info", "Starting endpoint attempt sequence (total candidates: " + 
               std::to_string(config_.endpoints.size()) + ")");

    for (size_t i = 0; i < config_.endpoints.size(); ++i) {
        const auto &ep = config_.endpoints[i];
        log("info", "Attempting emergency connection [" + std::to_string(i + 1) + "/" + 
                   std::to_string(config_.endpoints.size()) + "] to " + ep.ip + ":" + 
                   std::to_string(ep.port) + " (" + ep.protocol + ")...");
        
        if (vpnTunnel_->connect(ep.ip, ep.port, ep.protocol)) {
            log("info", "Successfully established VPN tunnel via " + ep.ip + ":" + std::to_string(ep.port));
            return true;
        }

        log("warn", "Endpoint " + ep.ip + ":" + std::to_string(ep.port) + " connection attempt failed. Advancing to next...");
    }

    log("error", "All emergency endpoints exhausted without a successful connection.");
    return false;
}

bool EmergencyConnectProxy::setupProxy() {
    proxyServer_ = new ProxyServer();
    
    if (!proxyServer_->start(config_.bindAddress, config_.proxyPort)) {
        log("error", "Failed to start proxy server on port " + std::to_string(config_.proxyPort));
        return false;
    }
    
    return true;
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
    
    return true;
}
