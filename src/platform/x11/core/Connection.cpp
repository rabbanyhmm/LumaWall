#include "Connection.hpp"
#include <core/Logging.hpp>

namespace luma::platform::x11::core {

std::expected<void, ConnectionError> Connection::connect(const std::string& displayName) {
    int screenp = 0;
    xcb_connection_t* conn = xcb_connect(displayName.empty() ? nullptr : displayName.c_str(), &screenp);
    
    if (xcb_connection_has_error(conn)) {
        spdlog::error("[X11] Failed to connect to X server");
        xcb_disconnect(conn);
        return std::unexpected(ConnectionError::FailedToConnect);
    }
    
    m_conn.reset(conn);
    m_defaultScreen = screenp;
    spdlog::info("[X11] Connected to X server on screen {}", screenp);
    
    return {};
}

void Connection::disconnect() {
    m_conn.reset();
}

} // namespace luma::platform::x11::core
