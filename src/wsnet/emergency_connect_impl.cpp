#include "emergency_connect_impl.h"
#include "../utils/logger.h"

#include <thread>
#include <random>
#include <algorithm>
#include <chrono>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <arpa/inet.h>
#endif

namespace wsnet {

EmergencyConnectImpl::EmergencyConnectImpl() {}

std::string EmergencyConnectImpl::ovpnConfig() const {
    return R"(client
dev tun
nobind
auth-user-pass
reneg-sec 432000
resolv-retry infinite
auth SHA512
cipher AES-256-GCM
data-ciphers AES-256-GCM:AES-128-GCM:CHACHA20-POLY1305
data-ciphers-fallback AES-256-GCM
connect-retry-max 1
connect-timeout 8
server-poll-timeout 6
verb 2
mute-replay-warnings
remote-cert-tls server
persist-key
persist-tun
key-direction 1
<ca>
-----BEGIN CERTIFICATE-----
MIIF3DCCA8SgAwIBAgIJAMsOivWTmu9fMA0GCSqGSIb3DQEBCwUAMHsxCzAJBgNV
BAYTAkNBMQswCQYDVQQIDAJPTjEQMA4GA1UEBwwHVG9yb250bzEbMBkGA1UECgwS
V2luZHNjcmliZSBMaW1pdGVkMRMwEQYDVQQLDApPcGVyYXRpb25zMRswGQYDVQQD
DBJXaW5kc2NyaWJlIE5vZGUgQ0EwHhcNMTYwMzA5MDMyNjIwWhcNNDAxMDI5MDMy
NjIwWjB7MQswCQYDVQQGEwJDQTELMAkGA1UECAwCT04xEDAOBgNVBAcMB1Rvcm9u
dG8xGzAZBgNVBAoMEldpbmRzY3JpYmUgTGltaXRlZDETMBEGA1UECwwKT3BlcmF0
aW9uczEbMBkGA1UEAwwSV2luZHNjcmliZSBOb2RlIENBMIICIjANBgkqhkiG9w0B
AQEFAAOCAg8AMIICCgKCAgEAruBtLR1Vufd71LeQEqChgHS4AQJ0fSRner0gmZPE
r2TL5uWboOEWXFFoEUTthF+P/N8yy3xRZ8HhG/zKlmJ1xw+7KZRbTADD6shJPj3/
uvTIO80sU+9LmsyKSWuPhQ1NkgNA7rrMTfz9eHJ2MVDs4XCpYWyX9iuAQrHSY6aP
q+4TpCbUgprkM3Gwjh9RSt9IoDoc4CF2bWSaVepUcL9yz/SXLPzFx2OT9rFrDhL3
ryHRzJQ/tA+VD8A7lo8bhOcDqiXgEFmVOZNMLw+r167Qq1Ck7X86yr2mnW/6HK2g
JOvY0/SPKukfGJAiYZKdG+fe4ekyYcAVhDfPJg7rF9wUqPwUzejJyAs1K18JwX94
Y8fnD6vQobjpC3qfHtwQP7Uj2AcI6QC8ytWDegV6UIkHXAMXBQSX5suSQoE11deG
32cy7nyp5vhgy31rTyNoopqlcCAhPm6k0jVVQbvXhLcpTSL8iCCoMdrP28i/xsfv
ktBAkl5giHMdK6hxqWgPI+Bx9uPIhRp3fJ2z8AgFm8g1ARB2ZzQ+OZZ2RUIkJuUK
hi2kUhgKSAQ+eF89aoqDjp/J1miZqGRzt4DovSZfQOeL01RkKHEibAPYCfgHG2ZS
woLoeaxE2vNZiX4dpXiOQYTOIXOwEPZzPvfTQf9T4Kxvx3jzQnt3PzjlMCqKk3Ai
pm8CAwEAAaNjMGEwHQYDVR0OBBYEFEH2v9F2z938Ebngsj9RkVSSgs45MB8GA1Ud
IwQYMBaAFEH2v9F2z938Ebngsj9RkVSSgs45MA8GA1UdEwEB/wQFMAMBAf8wDgYD
VR0PAQH/BAQDAgGGMA0GCSqGSIb3DQEBCwUAA4ICAQAgI6NgYkVo5rB6yKStgHjj
ZsINsgEvoMuHwkM0YaV22XtKNiHdsiOmY/PGCRemFobTEHk5XHcvcOTWv/D1qVf8
fI21WAoNQVH7h8KEsr4uMGKCB6Lu8l6xALXRMjo1xb6JKBWXwIAzUu691rUD2exT
1E+A5t+xw+gzqV8rWTMIoUaH7O1EKjN6ryGW71Khiik8/ETrP3YT32ZbS2P902iM
Kw9rpmuS0wWhnO5k/iO/6YNA1ZMV5JG5oZvZQYEDk7enLD9HvqazofMuy/Sz/n62
ZCDdQsnabzxl04wwv5Y3JZbV/6bOM520GgdJEoDxviY05ax2Mz05otyBzrAVjFw9
RZt/Ls8ATifu9BusZ2ootvscdIuE3x+ZCl5lvANcFEnvgGw0qpCeASLpsfxwq1dR
gIn7BOiTauFv4eoeFAQvCD+l+EKGWKu3M2y19DgYX94N2+Xs2bwChroaO5e4iFem
MLMuWKZvYgnqS9OAtRSYWbNX/wliiPz7u13yj+qSWgMfu8WPYNQlMZJXuGWUvKLE
XCUExlu7/o8D4HpsVs30E0pUdaqN0vExB1KegxPWWrmLcYnPG3knXpkC3ZBZ5P/e
l/2eyhZRy9ydiITF8gM3L08E8aeqvzZMw2FDSmousydIzlXgeS5VuEf+lUFA2h8o
ZYGQgrLt+ot8MbLhJlkp4Q==
-----END CERTIFICATE-----
</ca>
<tls-auth>
-----BEGIN OpenVPN Static key V1-----
5801926a57ac2ce27e3dfd1dd6ef8204
2d82bd4f3f0021296f57734f6f1ea714
a6623845541c4b0c3dea0a050fe6746c
b66dfab14cda27e5ae09d7c155aa554f
399fa4a863f0e8c1af787e5c602a801d
3a2ec41e395a978d56729457fe6102d7
d9e9119aa83643210b33c678f9d4109e
3154ac9c759e490cb309b319cf708cae
83ddadc3060a7a26564d1a24411cd552
fe6620ea16b755697a4fc5e6e9d0cfc0
c5c4a1874685429046a424c026db672e
4c2c492898052ba59128d46200b40f88
0027a8b6610a4d559bdc9346d33a0a6b
08e75c7fd43192b162bfd0aef0c716b3
1584827693f676f9a5047123466f0654
eade34972586b31c6ce7e395f4b478cb
-----END OpenVPN Static key V1-----
</tls-auth>
)";
}

std::string EmergencyConnectImpl::username() const {
    return "emergency";
}

std::string EmergencyConnectImpl::password() const {
    return "windscribe";
}

std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> EmergencyConnectImpl::getHardcodedEndpoints() const {
    std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> list;
    list.push_back(std::make_shared<EmergencyConnectEndpoint>("185.217.116.16", 1194, Protocol::kUdp));
    list.push_back(std::make_shared<EmergencyConnectEndpoint>("185.217.116.17", 1194, Protocol::kUdp));
    list.push_back(std::make_shared<EmergencyConnectEndpoint>("185.217.116.18", 1194, Protocol::kUdp));
    return list;
}

std::vector<std::string> EmergencyConnectImpl::resolveDomain(const std::string &domain) {
    std::vector<std::string> ips;
    
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    struct addrinfo *result = nullptr;
    int res = getaddrinfo(domain.c_str(), nullptr, &hints, &result);
    if (res == 0 && result != nullptr) {
        for (struct addrinfo *ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
            if (ptr->ai_family == AF_INET) {
                char ipStr[INET_ADDRSTRLEN];
                struct sockaddr_in *sockAddr = reinterpret_cast<struct sockaddr_in*>(ptr->ai_addr);
                if (inet_ntop(AF_INET, &(sockAddr->sin_addr), ipStr, sizeof(ipStr))) {
                    std::string ip(ipStr);
                    if (std::find(ips.begin(), ips.end(), ip) == ips.end()) {
                        ips.push_back(ip);
                    }
                }
            }
        }
        freeaddrinfo(result);
    }
    
    return ips;
}

std::shared_ptr<WSNetCancelableCallback> EmergencyConnectImpl::getIpEndpoints(WSNetEmergencyConnectCallback callback) {
    auto cancelable = std::make_shared<WSNetCancelableCallback>();

    std::thread([this, cancelable, callback]() {
        Logger::instance().info("[wsnet] Resolving remote emergency endpoints from econnect.windscribe.com...");

        std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> dynamicEndpoints;
        std::vector<std::string> resolvedIps = resolveDomain("econnect.windscribe.com");

        if (!resolvedIps.empty()) {
            Logger::instance().info("[wsnet] Successfully resolved " + std::to_string(resolvedIps.size()) + 
                                   " IP(s) from econnect.windscribe.com");
            
            // 随机打乱 IP 顺序
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(resolvedIps.begin(), resolvedIps.end(), g);

            for (const auto &ip : resolvedIps) {
                dynamicEndpoints.push_back(std::make_shared<EmergencyConnectEndpoint>(ip, 443, Protocol::kUdp));
                dynamicEndpoints.push_back(std::make_shared<EmergencyConnectEndpoint>(ip, 443, Protocol::kTcp));
            }
        } else {
            Logger::instance().warn("[wsnet] DNS resolution for econnect.windscribe.com yielded no IPs or was blocked. Using fallback emergency endpoints.");
        }

        // 追加官方硬编码后备端点
        auto fallbackEndpoints = getHardcodedEndpoints();
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(fallbackEndpoints.begin(), fallbackEndpoints.end(), g);

        std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> allEndpoints = std::move(dynamicEndpoints);
        allEndpoints.insert(allEndpoints.end(), fallbackEndpoints.begin(), fallbackEndpoints.end());

        if (cancelable->isCanceled()) {
            Logger::instance().info("[wsnet] Endpoint fetch request was canceled before delivery.");
            return;
        }

        Logger::instance().info("[wsnet] Delivering " + std::to_string(allEndpoints.size()) + " emergency endpoints to callback.");
        if (callback) {
            callback(allEndpoints);
        }
    }).detach();

    return cancelable;
}

std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> EmergencyConnectImpl::getIpEndpointsSync(int timeoutMs) {
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> resultEndpoints;

    auto cancelable = getIpEndpoints([&](const std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> &endpoints) {
        std::unique_lock<std::mutex> lock(mtx);
        resultEndpoints = endpoints;
        done = true;
        cv.notify_one();
    });

    std::unique_lock<std::mutex> lock(mtx);
    if (!cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&done]() { return done; })) {
        Logger::instance().warn("[wsnet] getIpEndpointsSync timed out after " + std::to_string(timeoutMs) + "ms. Canceling and using hardcoded fallbacks.");
        cancelable->cancel();
        return getHardcodedEndpoints();
    }

    return resultEndpoints;
}

} // namespace wsnet

