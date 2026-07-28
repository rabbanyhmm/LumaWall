#pragma once

#include <xcb/xcb.h>

namespace luma::platform::x11::wrapper {

class ScopedPixmap {
public:
    ScopedPixmap(xcb_connection_t* conn = nullptr, xcb_pixmap_t pixmap = XCB_NONE)
        : m_conn(conn), m_pixmap(pixmap) {}

    ~ScopedPixmap() {
        if (m_conn && m_pixmap != XCB_NONE) {
            xcb_free_pixmap(m_conn, m_pixmap);
        }
    }

    ScopedPixmap(const ScopedPixmap&) = delete;
    ScopedPixmap& operator=(const ScopedPixmap&) = delete;

    ScopedPixmap(ScopedPixmap&& other) noexcept 
        : m_conn(other.m_conn), m_pixmap(other.m_pixmap) {
        other.m_pixmap = XCB_NONE;
    }

    ScopedPixmap& operator=(ScopedPixmap&& other) noexcept {
        if (this != &other) {
            if (m_conn && m_pixmap != XCB_NONE) {
                xcb_free_pixmap(m_conn, m_pixmap);
            }
            m_conn = other.m_conn;
            m_pixmap = other.m_pixmap;
            other.m_pixmap = XCB_NONE;
        }
        return *this;
    }

    xcb_pixmap_t get() const { return m_pixmap; }
    operator xcb_pixmap_t() const { return m_pixmap; }

    void reset(xcb_connection_t* conn, xcb_pixmap_t pixmap) {
        if (m_conn && m_pixmap != XCB_NONE) {
            xcb_free_pixmap(m_conn, m_pixmap);
        }
        m_conn = conn;
        m_pixmap = pixmap;
    }

private:
    xcb_connection_t* m_conn;
    xcb_pixmap_t m_pixmap;
};

} // namespace luma::platform::x11::wrapper
