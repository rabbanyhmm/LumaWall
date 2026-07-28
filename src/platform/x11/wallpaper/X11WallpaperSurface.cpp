#include "X11WallpaperSurface.hpp"
#include <core/Logging.hpp>

namespace luma::platform::x11::wallpaper {

X11WallpaperSurface::X11WallpaperSurface(xcb_connection_t* conn, xcb_screen_t* screen, core::AtomCache* atomCache)
    : m_conn(conn), m_screen(screen), m_atomCache(atomCache) {}

bool X11WallpaperSurface::create(std::shared_ptr<IMonitor> monitor) {
    m_window = std::make_unique<DesktopWindow>(m_conn, m_screen, m_atomCache);
    return m_window->create(monitor->getWidth(), monitor->getHeight());
}

void X11WallpaperSurface::destroy() {
    m_window.reset();
}

void X11WallpaperSurface::resize(uint32_t width, uint32_t height) {
    if (m_window) {
        uint32_t values[] = { width, height };
        xcb_configure_window(m_conn, m_window->get(), XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
        xcb_flush(m_conn);
    }
}

void X11WallpaperSurface::present() {
}

void X11WallpaperSurface::pause() {}
void X11WallpaperSurface::resume() {}

void X11WallpaperSurface::show() {
    if (m_window) m_window->show();
}

void X11WallpaperSurface::hide() {
    if (m_window) m_window->hide();
}

} // namespace luma::platform::x11::wallpaper
