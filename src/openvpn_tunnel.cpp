#include "openvpn_tunnel.h"
#include "utils/logger.h"
#include "utils/process.h"
#include "wsnet/WSNet.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
#else
    #include <unistd.h>
    #include <signal.h>
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/stat.h>
    #include <sys/wait.h>
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
    
    // 轮询校验 OpenVPN 是否握手成功并分配到虚拟 IP（最多 8 秒）
    ProcessManager pm;
    for (int i = 0; i < 80; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (!pm.isProcessRunning(processPid_)) {
            Logger::instance().warn("[VPN] OpenVPN process terminated prematurely for " + ip + ":" + std::to_string(port) +
                                    ". (If AUTH_FAILED occurred, please specify valid credentials via -u/--username and -P/--password)");
            stopOpenVPNProcess();
            return false;
        }

        // 检查是否已分配到虚拟网卡和有效 IP
        std::string currentIp = getTunnelIP();
        if (!currentIp.empty()) {
            connected_ = true;
            if (statusCallback_) {
                statusCallback_("Connected to " + ip + ":" + std::to_string(port) + " (Tunnel IP: " + currentIp + ")");
            }
            return true;
        }
    }

    Logger::instance().warn("[VPN] Connection to " + ip + ":" + std::to_string(port) + " timed out without tunnel establishment.");
    stopOpenVPNProcess();
    return false;
}

bool OpenVPNTunnel::disconnect() {
    if (processPid_ != -1) {
        stopOpenVPNProcess();
    }
    
    if (!connected_) {
        return true;
    }
    connected_ = false;

#ifdef _WIN32
    char tempDir[MAX_PATH];
    if (GetTempPathA(MAX_PATH, tempDir) > 0) {
        DeleteFileA((std::string(tempDir) + "emergency_ovpn_temp.conf").c_str());
        DeleteFileA((std::string(tempDir) + "emergency_ovpn_auth.txt").c_str());
    }
#else
    unlink("/tmp/emergency_ovpn_temp.conf");
    unlink("/tmp/emergency_ovpn_auth.txt");
#endif

    if (statusCallback_) {
        statusCallback_("Disconnected");
    }
    
    return true;
}

bool OpenVPNTunnel::isConnected() const {
    return connected_;
}

std::string OpenVPNTunnel::getTunnelInterface() const {
#ifdef _WIN32
    ULONG bufLen = 15000;
    std::vector<BYTE> buffer(bufLen);
    PIP_ADAPTER_ADDRESSES addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, addresses, &bufLen) == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES curr = addresses; curr != nullptr; curr = curr->Next) {
            std::wstring desc = curr->Description ? curr->Description : L"";
            std::wstring name = curr->FriendlyName ? curr->FriendlyName : L"";
            std::string sDesc(desc.begin(), desc.end());
            std::string sName(name.begin(), name.end());
            if (sDesc.find("TAP") != std::string::npos || sDesc.find("Wintun") != std::string::npos ||
                sName.find("TAP") != std::string::npos || sName.find("Wintun") != std::string::npos ||
                sDesc.find("Windscribe") != std::string::npos) {
                return sName;
            }
        }
    }
    return "";
#else
    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == 0) {
        for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr) continue;
            std::string name(ifa->ifa_name);
            if ((name.rfind("tun", 0) == 0 || name.rfind("utun", 0) == 0) && (ifa->ifa_flags & IFF_UP)) {
                std::string res = name;
                freeifaddrs(ifaddr);
                return res;
            }
        }
        freeifaddrs(ifaddr);
    }
    return "";
#endif
}

std::string OpenVPNTunnel::getTunnelIP() const {
#ifdef _WIN32
    ULONG bufLen = 15000;
    std::vector<BYTE> buffer(bufLen);
    PIP_ADAPTER_ADDRESSES addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, addresses, &bufLen) == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES curr = addresses; curr != nullptr; curr = curr->Next) {
            std::wstring desc = curr->Description ? curr->Description : L"";
            std::wstring name = curr->FriendlyName ? curr->FriendlyName : L"";
            std::string sDesc(desc.begin(), desc.end());
            std::string sName(name.begin(), name.end());
            if (sDesc.find("TAP") != std::string::npos || sDesc.find("Wintun") != std::string::npos ||
                sName.find("TAP") != std::string::npos || sName.find("Wintun") != std::string::npos ||
                sDesc.find("Windscribe") != std::string::npos) {
                for (PIP_ADAPTER_UNICAST_ADDRESS u = curr->FirstUnicastAddress; u != nullptr; u = u->Next) {
                    if (u->Address.lpSockaddr->sa_family == AF_INET) {
                        char ipStr[INET_ADDRSTRLEN];
                        struct sockaddr_in *sa = reinterpret_cast<struct sockaddr_in*>(u->Address.lpSockaddr);
                        if (inet_ntop(AF_INET, &(sa->sin_addr), ipStr, sizeof(ipStr))) {
                            return std::string(ipStr);
                        }
                    }
                }
            }
        }
    }
    return "";
#else
    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == 0) {
        for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET) continue;
            std::string name(ifa->ifa_name);
            if ((name.rfind("tun", 0) == 0 || name.rfind("utun", 0) == 0) && (ifa->ifa_flags & IFF_UP)) {
                char host[NI_MAXHOST];
                int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
                if (s == 0) {
                    std::string ip(host);
                    freeifaddrs(ifaddr);
                    return ip;
                }
            }
        }
        freeifaddrs(ifaddr);
    }
    return "";
#endif
}

std::vector<std::string> OpenVPNTunnel::getDnsServers() const {
    return {"8.8.8.8", "8.8.4.4"};
}

void OpenVPNTunnel::setStatusCallback(StatusCallback callback) {
    statusCallback_ = callback;
}

std::string OpenVPNTunnel::generateTempConfigFile(const std::string &ip, uint16_t port, const std::string &protocol) {
#ifdef _WIN32
    char tempDir[MAX_PATH];
    GetTempPathA(MAX_PATH, tempDir);
    std::string tempPath = std::string(tempDir) + "emergency_ovpn_temp.conf";
#else
    std::string tempPath = "/tmp/emergency_ovpn_temp.conf";
#endif
    std::ofstream out(tempPath);
    if (!configPath_.empty()) {
        std::ifstream in(configPath_);
        if (in.is_open()) {
            out << in.rdbuf();
        } else {
            out << wsnet::emergencyConnect()->ovpnConfig();
        }
    } else {
        out << wsnet::emergencyConnect()->ovpnConfig();
    }
    out << "\nremote " << ip << " " << port << " " << protocol << "\n";
    out << "connect-retry-max 1\n";
    out << "connect-timeout 8\n";
    out << "server-poll-timeout 6\n";
    out.close();
    return tempPath;
}

bool OpenVPNTunnel::startOpenVPNProcess(const std::string &configFile) {
    // 启动 OpenVPN 进程
#ifdef _WIN32
    char tempDir[MAX_PATH];
    GetTempPathA(MAX_PATH, tempDir);
    std::string authFile = std::string(tempDir) + "emergency_ovpn_auth.txt";
#else
    std::string authFile = "/tmp/emergency_ovpn_auth.txt";
#endif
    std::ofstream auth(authFile);
    if (!username_.empty()) {
        auth << username_ << "\n" << password_ << "\n";
    } else {
        auth << wsnet::emergencyConnect()->username() << "\n" << wsnet::emergencyConnect()->password() << "\n";
    }
    auth.close();

#ifndef _WIN32
    chmod(authFile.c_str(), 0600);
#endif

    ProcessManager pm;
    return pm.startProcess("openvpn", {"--config", configFile, "--auth-user-pass", authFile, "--connect-retry-max", "1", "--connect-timeout", "8"}, processPid_);
}

bool OpenVPNTunnel::stopOpenVPNProcess() {
    if (processPid_ == -1) {
        return false;
    }
    
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processPid_);
    if (hProcess) {
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
    }
    processPid_ = -1;
    return true;
#else
    kill(processPid_, SIGTERM);
    int status = 0;
    waitpid(processPid_, &status, WNOHANG);
    processPid_ = -1;
    return true;
#endif
}
