#include "DisplayConnection.hpp"
#include <wayland-client.h>

namespace luma::platform::wayland::core {

std::expected<void, DisplayError> DisplayConnection::connect(const std::string& displayName) {
    if (m_display) {
        return std::unexpected(DisplayError::AlreadyConnected);
    }
    
    wl_display* display = wl_display_connect(displayName.empty() ? nullptr : displayName.c_str());
    if (!display) {
        return std::unexpected(DisplayError::ConnectionFailed);
    }
    
    m_display.reset(display);
    return {};
}

void DisplayConnection::disconnect() {
    m_display.reset();
}

int DisplayConnection::getFd() const noexcept {
    return m_display ? wl_display_get_fd(m_display.get()) : -1;
}

void DisplayConnection::roundtrip() {
    if (m_display) {
        wl_display_roundtrip(m_display.get());
    }
}

int DisplayConnection::dispatchPending() {
    if (m_display) {
        return wl_display_dispatch_pending(m_display.get());
    }
    return -1;
}

int DisplayConnection::flush() {
    if (m_display) {
        return wl_display_flush(m_display.get());
    }
    return -1;
}

} // namespace luma::platform::wayland::core
