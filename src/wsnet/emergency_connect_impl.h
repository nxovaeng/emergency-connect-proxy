#pragma once

#include "WSNetEmergencyConnect.h"
#include <mutex>
#include <condition_variable>

namespace wsnet {

class EmergencyConnectImpl : public WSNetEmergencyConnect {
public:
    EmergencyConnectImpl();
    ~EmergencyConnectImpl() override = default;

    std::string ovpnConfig() const override;
    std::string username() const override;
    std::string password() const override;

    std::shared_ptr<WSNetCancelableCallback> getIpEndpoints(WSNetEmergencyConnectCallback callback) override;
    std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> getIpEndpointsSync(int timeoutMs = 5000) override;

private:
    std::vector<std::string> resolveDomain(const std::string &domain);
    std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> getHardcodedEndpoints() const;
};

} // namespace wsnet

