#include "WSNet.h"
#include "emergency_connect_impl.h"
#include "../utils/logger.h"
#include <mutex>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

namespace wsnet {

static std::shared_ptr<WSNet> s_wsnetInstance = nullptr;
static std::mutex s_instanceMutex;

std::shared_ptr<WSNet> WSNet::instance() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (!s_wsnetInstance) {
        s_wsnetInstance = std::make_shared<WSNet>();
    }
    return s_wsnetInstance;
}

bool WSNet::initialize() {
    if (initialized_) {
        return true;
    }

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    emergencyConnect_ = std::make_shared<EmergencyConnectImpl>();
    initialized_ = true;

    Logger::instance().info("[wsnet] WSNet subsystem initialized successfully.");
    return true;
}

bool WSNet::initialize(const std::string &basePlatform, const std::string &platformName, const std::string &appVersion,
                       const std::string &deviceId, const std::string &openVpnVersion, const std::string &sessionTypeId,
                       bool isUseStagingDomains, const std::string &language, const std::string &persistentSettings,
                       WSNetLoggerFunction loggerFunction, bool debugLog, const std::string &amneziawgVersion) {
    (void)basePlatform;
    (void)platformName;
    (void)appVersion;
    (void)deviceId;
    (void)openVpnVersion;
    (void)sessionTypeId;
    (void)isUseStagingDomains;
    (void)language;
    (void)persistentSettings;
    (void)debugLog;
    (void)amneziawgVersion;

    if (loggerFunction) {
        loggerFunction("[wsnet] Initializing with custom logger callback");
    }

    return instance()->initialize();
}

void WSNet::cleanup() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (s_wsnetInstance) {
        s_wsnetInstance->emergencyConnect_.reset();
        s_wsnetInstance->initialized_ = false;
        s_wsnetInstance.reset();
#ifdef _WIN32
        WSACleanup();
#endif
        Logger::instance().info("[wsnet] WSNet subsystem cleaned up.");
    }
}

bool WSNet::isValid() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    return s_wsnetInstance != nullptr && s_wsnetInstance->initialized_;
}

std::shared_ptr<WSNetEmergencyConnect> WSNet::emergencyConnect() {
    if (!initialized_) {
        initialize();
    }
    return emergencyConnect_;
}

} // namespace wsnet

