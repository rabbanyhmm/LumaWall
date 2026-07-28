#pragma once
#include <platform/x11/wrapper/ScopedWindow.hpp>
#include <platform/x11/core/AtomCache.hpp>
#include <memory>

namespace luma::platform::x11::wallpaper {

class DesktopWindow {
public:
    DesktopWindow(xcb_connection_t* conn, xcb_screen_t* screen, core::AtomCache* atomCache);
    ~DesktopWindow() = default;

    bool create(uint32_t width, uint32_t height);
    void show();
    void hide();
    xcb_window_t get() const { return m_window.get(); }

private:
    xcb_connection_t* m_conn;
    xcb_screen_t* m_screen;
    core::AtomCache* m_atomCache;
    wrapper::ScopedWindow m_window;
};

} // namespace luma::platform::x11::wallpaper
