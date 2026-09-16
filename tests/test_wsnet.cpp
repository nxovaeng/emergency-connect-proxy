#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <atomic>
#include "wsnet/WSNet.h"

int main() {
    std::cout << "[TEST] Starting WSNet unit tests..." << std::endl;

    // Test 1: WSNet::instance()->initialize()
    std::cout << "[TEST 1] Testing WSNet::instance()->initialize()..." << std::endl;
    bool initOk = WSNet::instance()->initialize();
    assert(initOk && "WSNet::instance()->initialize() should return true");
    assert(WSNet::isValid() && "WSNet::isValid() should return true");

    // Test 2: wsnet::emergencyConnect()->getIpEndpoints(callback)
    std::cout << "[TEST 2] Testing wsnet::emergencyConnect()->getIpEndpoints(callback)..." << std::endl;
    std::atomic<bool> callbackCalled{false};
    std::atomic<size_t> endpointCount{0};

    auto cancelable = wsnet::emergencyConnect()->getIpEndpoints([&](const std::vector<std::shared_ptr<wsnet::WSNetEmergencyConnectEndpoint>> &endpoints) {
        callbackCalled.store(true);
        endpointCount.store(endpoints.size());
        for (const auto &ep : endpoints) {
            assert(!ep->ip().empty());
            assert(ep->port() > 0);
        }
    });

    assert(cancelable != nullptr && "Cancelable callback should not be null");

    // Wait for callback (up to 3 seconds)
    for (int i = 0; i < 30; ++i) {
        if (callbackCalled.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    assert(callbackCalled.load() && "Callback should have been called");
    assert(endpointCount.load() > 0 && "Should receive at least 1 endpoint");
    std::cout << "[TEST 2] Received " << endpointCount.load() << " endpoints successfully." << std::endl;

    // Test 3: WSNetCancelableCallback cancel()
    std::cout << "[TEST 3] Testing WSNetCancelableCallback::cancel()..." << std::endl;
    std::atomic<bool> canceledCallbackCalled{false};
    auto cancelable2 = wsnet::emergencyConnect()->getIpEndpoints([&](const auto &) {
        canceledCallbackCalled.store(true);
    });
    cancelable2->cancel();
    assert(cancelable2->isCanceled() && "Callback should be marked as canceled");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    assert(!canceledCallbackCalled.load() && "Canceled callback must not be invoked");
    std::cout << "[TEST 3] Cancelation verified successfully." << std::endl;

    // Test 4: wsnet::emergencyConnect()->getIpEndpointsSync()
    std::cout << "[TEST 4] Testing wsnet::emergencyConnect()->getIpEndpointsSync()..." << std::endl;
    auto syncEndpoints = wsnet::emergencyConnect()->getIpEndpointsSync(2000);
    assert(!syncEndpoints.empty() && "getIpEndpointsSync should return non-empty endpoints");
    std::cout << "[TEST 4] Sync retrieved " << syncEndpoints.size() << " endpoints." << std::endl;

    // Test 5: Default OVPN Config and credentials
    std::cout << "[TEST 5] Testing emergency credentials and OVPN template..." << std::endl;
    assert(!wsnet::emergencyConnect()->username().empty());
    assert(!wsnet::emergencyConnect()->password().empty());
    assert(wsnet::emergencyConnect()->ovpnConfig().find("-----BEGIN CERTIFICATE-----") != std::string::npos);
    assert(wsnet::emergencyConnect()->ovpnConfig().find("-----BEGIN OpenVPN Static key V1-----") != std::string::npos);
    std::cout << "[TEST 5] Embedded credentials and certificates verified." << std::endl;

    std::cout << "[TEST] All WSNet tests passed successfully!" << std::endl;
    return 0;
}

