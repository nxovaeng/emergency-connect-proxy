#include "system_proxy.h"
#include <iostream>
#include <cstdlib>

SystemProxy::SystemProxy() {}

SystemProxy::~SystemProxy() {
    restore();
}

bool SystemProxy::setProxy(const std::string &proxyUrl) {
#ifdef _WIN32
    return setWindowsProxy(proxyUrl);
#elif __APPLE__
    return setMacProxy(proxyUrl);
#else
    return setLinuxProxy(proxyUrl);
#endif
}

bool SystemProxy::restore() {
#ifdef _WIN32
    return restoreWindowsProxy();
#elif __APPLE__
    return restoreMacProxy();
#else
    return restoreLinuxProxy();
#endif
}

std::string SystemProxy::getCurrentProxy() const {
    // 实现获取当前系统代理的逻辑
    return "";
}

#ifdef _WIN32
bool SystemProxy::setWindowsProxy(const std::string &proxyUrl) {
    std::string cmd = "netsh winhttp set proxy proxy-server=\"" + proxyUrl + "\"";
    return system(cmd.c_str()) == 0;
}

bool SystemProxy::restoreWindowsProxy() {
    return system("netsh winhttp reset proxy") == 0;
}
#elif __APPLE__
bool SystemProxy::setMacProxy(const std::string &proxyUrl) {
    // macOS 系统代理设置命令
    // 需要解析代理 URL 获取 IP 和端口
    return true;
}

bool SystemProxy::restoreMacProxy() {
    return true;
}
#else
bool SystemProxy::setLinuxProxy(const std::string &proxyUrl) {
    // Linux 环境变量设置
    return true;
}

bool SystemProxy::restoreLinuxProxy() {
    return true;
}
#endif
