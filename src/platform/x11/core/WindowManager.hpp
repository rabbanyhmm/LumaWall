#pragma once
#include <xcb/xcb.h>
#include <vector>

namespace luma::platform::x11::core {

class WindowManager {
public:
    static void setProperty(xcb_connection_t* conn, xcb_window_t window, xcb_atom_t property, xcb_atom_t type, uint8_t format, const void* data, uint32_t dataLen);
    static void setAtomProperty(xcb_connection_t* conn, xcb_window_t window, xcb_atom_t property, xcb_atom_t value);
    static void setAtomListProperty(xcb_connection_t* conn, xcb_window_t window, xcb_atom_t property, const std::vector<xcb_atom_t>& values);
};

} // namespace luma::platform::x11::core
