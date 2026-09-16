#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "WSNetEmergencyConnectEndpoint.h"
#include "WSNetCancelableCallback.h"

namespace wsnet {

typedef std::function<void(const std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> &endpoints)> WSNetEmergencyConnectCallback;

class WSNetEmergencyConnect {
public:
    virtual ~WSNetEmergencyConnect() = default;

    virtual std::string ovpnConfig() const = 0;
    virtual std::string username() const = 0;
    virtual std::string password() const = 0;

    // Returns a ready-to-use list of endpoints(ip, port, protocol), already randomized
    virtual std::shared_ptr<WSNetCancelableCallback> getIpEndpoints(WSNetEmergencyConnectCallback callback) = 0;

    // Helper synchronous version
    virtual std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> getIpEndpointsSync(int timeoutMs = 5000) = 0;
};

} // namespace wsnet

