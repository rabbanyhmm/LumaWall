#pragma once

#include <xcb/xcb.h>

namespace luma::platform::x11::wrapper {

class ScopedWindow {
public:
    ScopedWindow(xcb_connection_t* conn = nullptr, xcb_window_t win = XCB_NONE)
        : m_conn(conn), m_window(win) {}

    ~ScopedWindow() {
        if (m_conn && m_window != XCB_NONE) {
            xcb_destroy_window(m_conn, m_window);
        }
    }

    ScopedWindow(const ScopedWindow&) = delete;
    ScopedWindow& operator=(const ScopedWindow&) = delete;

    ScopedWindow(ScopedWindow&& other) noexcept 
        : m_conn(other.m_conn), m_window(other.m_window) {
        other.m_window = XCB_NONE;
    }

    ScopedWindow& operator=(ScopedWindow&& other) noexcept {
        if (this != &other) {
            if (m_conn && m_window != XCB_NONE) {
                xcb_destroy_window(m_conn, m_window);
            }
            m_conn = other.m_conn;
            m_window = other.m_window;
            other.m_window = XCB_NONE;
        }
        return *this;
    }

    xcb_window_t get() const { return m_window; }
    operator xcb_window_t() const { return m_window; }

    void reset(xcb_connection_t* conn, xcb_window_t win) {
        if (m_conn && m_window != XCB_NONE) {
            xcb_destroy_window(m_conn, m_window);
        }
        m_conn = conn;
        m_window = win;
    }

private:
    xcb_connection_t* m_conn;
    xcb_window_t m_window;
};

} // namespace luma::platform::x11::wrapper
