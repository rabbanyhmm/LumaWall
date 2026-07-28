#pragma once
#include <platform/x11/wrapper/ScopedConnection.hpp>
#include <expected>
#include <string>

namespace luma::platform::x11::core {

enum class ConnectionError {
    FailedToConnect,
    AuthenticationFailed,
    ConnectionClosed
};

class Connection {
public:
    Connection() = default;
    ~Connection() = default;

    std::expected<void, ConnectionError> connect(const std::string& displayName = "");
    void disconnect();

    xcb_connection_t* get() const { return m_conn.get(); }
    int getDefaultScreen() const { return m_defaultScreen; }

private:
    wrapper::ScopedConnection m_conn;
    int m_defaultScreen{0};
};

} // namespace luma::platform::x11::core
