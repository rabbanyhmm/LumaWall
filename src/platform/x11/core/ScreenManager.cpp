#include "ScreenManager.hpp"

namespace luma::platform::x11::core {

ScreenManager::ScreenManager(xcb_connection_t* conn, int defaultScreen) {
    const xcb_setup_t* setup = xcb_get_setup(conn);
    if (!setup) return;

    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    int currentScreen = 0;
    
    for (; iter.rem; xcb_screen_next(&iter), ++currentScreen) {
        m_screens.push_back(iter.data);
        if (currentScreen == defaultScreen) {
            m_defaultScreen = iter.data;
        }
    }
}

} // namespace luma::platform::x11::core
