#pragma once

#include <atomic>
#include <memory>
#include <functional>

namespace wsnet {

class WSNetCancelableCallback {
public:
    WSNetCancelableCallback() : canceled_(false) {}
    virtual ~WSNetCancelableCallback() = default;

    virtual void cancel() {
        canceled_.store(true);
    }

    virtual bool isCanceled() const {
        return canceled_.load();
    }

private:
    std::atomic<bool> canceled_{false};
};

} // namespace wsnet

