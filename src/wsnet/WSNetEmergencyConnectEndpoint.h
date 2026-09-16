#pragma once

#include <string>
#include <cstdint>
#include <memory>

namespace wsnet {

enum class Protocol {
    kUdp = 0,
    kTcp
};

class WSNetEmergencyConnectEndpoint {
public:
    virtual ~WSNetEmergencyConnectEndpoint() = default;

    virtual std::string ip() const = 0;
    virtual std::uint16_t port() const = 0;
    virtual Protocol protocol() const = 0;
};

class EmergencyConnectEndpoint : public WSNetEmergencyConnectEndpoint {
public:
    EmergencyConnectEndpoint(std::string ip, std::uint16_t port, Protocol protocol)
        : ip_(std::move(ip)), port_(port), protocol_(protocol) {}

    std::string ip() const override { return ip_; }
    std::uint16_t port() const override { return port_; }
    Protocol protocol() const override { return protocol_; }

private:
    std::string ip_;
    std::uint16_t port_;
    Protocol protocol_;
};

} // namespace wsnet

