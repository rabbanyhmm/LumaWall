#include "DesktopWindow.hpp"
#include <platform/x11/core/WindowManager.hpp>
#include <core/Logging.hpp>

namespace luma::platform::x11::wallpaper {

DesktopWindow::DesktopWindow(xcb_connection_t* conn, xcb_screen_t* screen, core::AtomCache* atomCache)
    : m_conn(conn), m_screen(screen), m_atomCache(atomCache) {}

bool DesktopWindow::create(uint32_t width, uint32_t height) {
    xcb_window_t window = xcb_generate_id(m_conn);

    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t values[2];
    values[0] = m_screen->black_pixel;
    values[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(m_conn,
        XCB_COPY_FROM_PARENT,
        window,
        m_screen->root,
        0, 0,
        static_cast<uint16_t>(width), static_cast<uint16_t>(height),
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        m_screen->root_visual,
        mask, values);

    m_window.reset(m_conn, window);

    // Set window title to "LumaWall-RendererWindow-X11" so the GNOME extension can find it securely
    const char* title = "LumaWall-RendererWindow-X11";
    xcb_change_property(m_conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(title), title);

    // CRITICAL: Use _NET_WM_WINDOW_TYPE_NORMAL so Mutter creates a compositor actor for us.
    // DESKTOP-type windows do NOT get compositor actors in Mutter/GNOME Shell, which means
    // Clutter.Clone cannot clone them. We use NORMAL type with special states to achieve the
    // same visual result (always-below, hidden from taskbar/alt-tab/overview) while getting
    // a proper WindowActor that the extension can clone into the background.
    xcb_atom_t typeAtom = m_atomCache->get("_NET_WM_WINDOW_TYPE");
    xcb_atom_t normalTypeAtom = m_atomCache->get("_NET_WM_WINDOW_TYPE_NORMAL");

    if (typeAtom != XCB_NONE && normalTypeAtom != XCB_NONE) {
        core::WindowManager::setAtomProperty(m_conn, window, typeAtom, normalTypeAtom);
        spdlog::info("[X11] Set window type to _NET_WM_WINDOW_TYPE_NORMAL (required for compositor actor)");
    }

    // Set WM_HINTS with input=False so the window manager never focuses this window.
    // This completely prevents GNOME Shell from placing "LumaWall" with a combobox in the Top Bar.
    // The struct is: { flags, input, initial_state, icon_pixmap, icon_window, icon_x, icon_y, icon_mask, window_group }
    uint32_t wm_hints[9] = {
        1, // flags: XCB_WM_HINT_INPUT (1 << 0)
        0, // input: False
        0, 0, 0, 0, 0, 0, 0
    };
    xcb_change_property(m_conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_HINTS, XCB_ATOM_WM_HINTS, 32, 9, wm_hints);

    // Set _NET_WM_NAME (UTF8) in addition to WM_NAME for robust title matching
    xcb_atom_t netWmName = m_atomCache->get("_NET_WM_NAME");
    xcb_atom_t utf8String = m_atomCache->get("UTF8_STRING");
    if (netWmName != XCB_NONE && utf8String != XCB_NONE) {
        xcb_change_property(m_conn, XCB_PROP_MODE_REPLACE, window, netWmName, utf8String, 8, strlen(title), title);
    }

    // Set _NET_WM_STATE to: BELOW + SKIP_TASKBAR + SKIP_PAGER + STICKY
    xcb_atom_t stateAtom        = m_atomCache->get("_NET_WM_STATE");
    xcb_atom_t stateBelowAtom   = m_atomCache->get("_NET_WM_STATE_BELOW");
    xcb_atom_t stateSkipTaskbar = m_atomCache->get("_NET_WM_STATE_SKIP_TASKBAR");
    xcb_atom_t stateSkipPager   = m_atomCache->get("_NET_WM_STATE_SKIP_PAGER");
    xcb_atom_t stateStickyAtom  = m_atomCache->get("_NET_WM_STATE_STICKY");

    if (stateAtom != XCB_NONE) {
        std::vector<xcb_atom_t> states;
        if (stateBelowAtom   != XCB_NONE) states.push_back(stateBelowAtom);
        if (stateSkipTaskbar != XCB_NONE) states.push_back(stateSkipTaskbar);
        if (stateSkipPager   != XCB_NONE) states.push_back(stateSkipPager);
        if (stateStickyAtom  != XCB_NONE) states.push_back(stateStickyAtom);
        core::WindowManager::setAtomListProperty(m_conn, window, stateAtom, states);
        spdlog::info("[X11] Set window states: BELOW + SKIP_TASKBAR + SKIP_PAGER + STICKY");
    }

    // Remove ALL window decorations (title bar, borders, resize handles).
    // _MOTIF_WM_HINTS with decorations=0 is the standard way to tell the WM
    // "do not decorate this window". Without this, GNOME/Mutter adds a title
    // bar and borders to our NORMAL-type window, causing the flicker/combobox
    // the user sees briefly before the extension hides the window.
    // Format: {flags, functions, decorations, input_mode, status}
    // flags = MWM_HINTS_DECORATIONS (1 << 1 = 2), decorations = 0 (none)
    xcb_atom_t motifHintsAtom = m_atomCache->get("_MOTIF_WM_HINTS");
    if (motifHintsAtom != XCB_NONE) {
        uint32_t motifHints[5] = {2, 0, 0, 0, 0};  // flags=MWM_HINTS_DECORATIONS, rest=0
        xcb_change_property(m_conn, XCB_PROP_MODE_REPLACE, window, motifHintsAtom,
                            motifHintsAtom, 32, 5, motifHints);
        spdlog::info("[X11] Set _MOTIF_WM_HINTS: decorations=none");
    }

    spdlog::info("[X11] Created DesktopWindow {}", window);
    return true;

}

void DesktopWindow::show() {
    xcb_map_window(m_conn, m_window.get());
    xcb_flush(m_conn);
}

void DesktopWindow::hide() {
    xcb_unmap_window(m_conn, m_window.get());
    xcb_flush(m_conn);
}

} // namespace luma::platform::x11::wallpaper
