#include <iostream>
#include <string>
#include <cstring>
#include <csignal>
#include <atomic>
#include "emergency_proxy.h"
#include "utils/logger.h"
#include "wsnet/WSNet.h"

static std::atomic<bool> g_running{true};

static void handleSignal(int signum) {
    (void)signum;
    g_running = false;
}

void printUsage(const char *programName) {
    std::cout << "Emergency Connect Proxy for Windscribe" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --start, -s              Start the proxy" << std::endl;
    std::cout << "  --stop, -S               Stop the proxy" << std::endl;
    std::cout << "  --status                 Check proxy status" << std::endl;
    std::cout << "  --fetch-endpoints        Fetch and list remote emergency endpoints via wsnet" << std::endl;
    std::cout << "  --no-auto-fetch          Disable automatic remote endpoint resolution" << std::endl;
    std::cout << "  --port PORT, -p PORT     Proxy port (default: 8888)" << std::endl;
    std::cout << "  --bind ADDR              Bind address (default: 127.0.0.1)" << std::endl;
    std::cout << "  --config FILE, -c FILE   Configuration file" << std::endl;
    std::cout << "  --log-level LEVEL        Log level (debug/info/warn/error)" << std::endl;
    std::cout << "  --log-file FILE          Log file path" << std::endl;
    std::cout << "  --help, -h               Show this help message" << std::endl;
    std::cout << "  --version, -v            Show version" << std::endl;
}

int main(int argc, char *argv[]) {
    bool startProxy = false;
    bool stopProxy = false;
    bool checkStatus = false;
    bool fetchEndpointsOnly = false;
    bool autoFetch = true;
    std::string proxyPort = "8888";
    std::string bindAddr = "127.0.0.1";
    std::string configFile;
    std::string logLevel = "info";
    std::string logFile;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--start" || arg == "-s") {
            startProxy = true;
        }
        else if (arg == "--stop" || arg == "-S") {
            stopProxy = true;
        }
        else if (arg == "--status") {
            checkStatus = true;
        }
        else if (arg == "--fetch-endpoints") {
            fetchEndpointsOnly = true;
        }
        else if (arg == "--no-auto-fetch") {
            autoFetch = false;
        }
        else if (arg == "--port" || arg == "-p") {
            if (i + 1 < argc) {
                proxyPort = argv[++i];
            }
        }
        else if (arg == "--bind") {
            if (i + 1 < argc) {
                bindAddr = argv[++i];
            }
        }
        else if (arg == "--config" || arg == "-c") {
            if (i + 1 < argc) {
                configFile = argv[++i];
            }
        }
        else if (arg == "--log-level") {
            if (i + 1 < argc) {
                logLevel = argv[++i];
            }
        }
        else if (arg == "--log-file") {
            if (i + 1 < argc) {
                logFile = argv[++i];
            }
        }
        else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "--version" || arg == "-v") {
            std::cout << "Emergency Connect Proxy v1.0.0" << std::endl;
            return 0;
        }
    }
    
    // 初始化日志
    Logger::instance().init(logFile, logLevel);
    
    // 初始化 WSNet 子系统
    WSNet::instance()->initialize();

    // 独立查询/拉取端点信息命令
    if (fetchEndpointsOnly) {
        std::cout << "Fetching emergency connect endpoints via wsnet::emergencyConnect()..." << std::endl;
        wsnet::emergencyConnect()->getIpEndpoints([](const std::vector<std::shared_ptr<wsnet::WSNetEmergencyConnectEndpoint>> &endpoints) {
            std::cout << "Successfully retrieved " << endpoints.size() << " emergency endpoint(s):" << std::endl;
            for (size_t i = 0; i < endpoints.size(); ++i) {
                std::cout << "  [" << (i + 1) << "] " << endpoints[i]->ip() << ":"
                          << endpoints[i]->port() << " ("
                          << (endpoints[i]->protocol() == wsnet::Protocol::kTcp ? "TCP" : "UDP")
                          << ")" << std::endl;
            }
        });
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return 0;
    }
    
    // 创建代理实例
    EmergencyConnectProxy proxy;
    
    // 设置日志回调
    proxy.setLogCallback([](const std::string &level, const std::string &message) {
        if (level == "debug") {
            Logger::instance().debug(message);
        } else if (level == "info") {
            Logger::instance().info(message);
        } else if (level == "warn") {
            Logger::instance().warn(message);
        } else if (level == "error") {
            Logger::instance().error(message);
        }
    });
    
    // 配置代理
    ProxyConfig config;
    config.proxyPort = std::stoi(proxyPort);
    config.bindAddress = bindAddr;
    config.autoFetchEndpoints = autoFetch;
    config.logLevel = logLevel;
    config.logFile = logFile;
    
    if (!proxy.initialize(config)) {
        std::cerr << "Failed to initialize proxy" << std::endl;
        return 1;
    }
    
    // 处理命令
    if (startProxy) {
        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);

        std::cout << "Starting proxy..." << std::endl;
        if (proxy.start()) {
            std::cout << "Proxy started on " << proxy.getProxyUrl() << std::endl;
            std::cout << "Press Ctrl+C to stop..." << std::endl;
            
            // 保持运行直到收到信号
            while (g_running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            std::cout << "\nStopping proxy gracefully..." << std::endl;
            proxy.stop();
            std::cout << "Proxy stopped." << std::endl;
        } else {
            std::cerr << "Failed to start proxy" << std::endl;
            return 1;
        }
    }
    else if (stopProxy) {
        if (proxy.stop()) {
            std::cout << "Proxy stopped" << std::endl;
            return 0;
        } else {
            std::cerr << "Failed to stop proxy" << std::endl;
            return 1;
        }
    }
    else if (checkStatus) {
        std::cout << proxy.getStatus() << std::endl;
        return 0;
    }
    else {
        printUsage(argv[0]);
        return 0;
    }
    
    return 0;
}
