#include "WindowManager.hpp"

namespace luma::platform::x11::core {

void WindowManager::setProperty(xcb_connection_t* conn, xcb_window_t window, xcb_atom_t property, xcb_atom_t type, uint8_t format, const void* data, uint32_t dataLen) {
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, property, type, format, dataLen, data);
}

void WindowManager::setAtomProperty(xcb_connection_t* conn, xcb_window_t window, xcb_atom_t property, xcb_atom_t value) {
    setProperty(conn, window, property, XCB_ATOM_ATOM, 32, &value, 1);
}

void WindowManager::setAtomListProperty(xcb_connection_t* conn, xcb_window_t window, xcb_atom_t property, const std::vector<xcb_atom_t>& values) {
    setProperty(conn, window, property, XCB_ATOM_ATOM, 32, values.data(), static_cast<uint32_t>(values.size()));
}

} // namespace luma::platform::x11::core
