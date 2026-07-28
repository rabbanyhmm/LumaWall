#pragma once

#include <xcb/xcb.h>
#include <memory>

namespace luma::platform::x11::wrapper {

struct ConnectionDeleter {
    void operator()(xcb_connection_t* c) const noexcept {
        if (c) xcb_disconnect(c);
    }
};

using ScopedConnection = std::unique_ptr<xcb_connection_t, ConnectionDeleter>;

} // namespace luma::platform::x11::wrapper
