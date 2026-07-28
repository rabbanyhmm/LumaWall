#pragma once

#include <platform/wayland/wrapper/ScopedProxy.hpp>
#include <string>
#include <expected>

namespace luma::platform::wayland::core {

enum class DisplayError {
    ConnectionFailed,
    AlreadyConnected
};

class DisplayConnection {
public:
    DisplayConnection() = default;
    ~DisplayConnection() = default;

    std::expected<void, DisplayError> connect(const std::string& displayName = "");
    void disconnect();

    wl_display* get() const noexcept { return m_display.get(); }
    int getFd() const noexcept;
    
    // Perform a roundtrip to synchronize with the server
    void roundtrip();
    
    // Dispatch pending events
    int dispatchPending();
    
    // Flush outgoing requests
    int flush();

private:
    wrapper::ScopedDisplay m_display;
};

} // namespace luma::platform::wayland::core
