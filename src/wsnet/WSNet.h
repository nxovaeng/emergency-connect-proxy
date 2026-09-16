#pragma once

#include <string>
#include <memory>
#include <functional>
#include "WSNetEmergencyConnect.h"
#include "WSNetEmergencyConnectEndpoint.h"
#include "WSNetCancelableCallback.h"

namespace wsnet {

typedef std::function<void(const std::string &)> WSNetLoggerFunction;

class WSNet {
public:
    virtual ~WSNet() = default;

    // 单例获取
    static std::shared_ptr<WSNet> instance();

    // 实例方法初始化: WSNet::instance()->initialize()
    virtual bool initialize();

    // 静态初始化方法（与官方 wsnet 参数签名兼容）
    static bool initialize(const std::string &basePlatform, const std::string &platformName, const std::string &appVersion,
                           const std::string &deviceId = "", const std::string &openVpnVersion = "", const std::string &sessionTypeId = "3",
                           bool isUseStagingDomains = false, const std::string &language = "en", const std::string &persistentSettings = "",
                           WSNetLoggerFunction loggerFunction = nullptr, bool debugLog = false, const std::string &amneziawgVersion = "");

    // 资源清理
    static void cleanup();

    // 有效性检查
    static bool isValid();

    // 获取紧急连接接口
    virtual std::shared_ptr<WSNetEmergencyConnect> emergencyConnect();

private:
    std::shared_ptr<WSNetEmergencyConnect> emergencyConnect_;
    bool initialized_ = false;
};

// 命名空间顶层快捷函数: wsnet::emergencyConnect()
inline std::shared_ptr<WSNetEmergencyConnect> emergencyConnect() {
    return WSNet::instance()->emergencyConnect();
}

} // namespace wsnet

// 导出至全局命名空间，保证 WSNet::instance()->initialize() 直接可用
using wsnet::WSNet;
using wsnet::Protocol;

