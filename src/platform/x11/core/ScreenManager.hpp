#pragma once
#include <xcb/xcb.h>
#include <vector>

namespace luma::platform::x11::core {

class ScreenManager {
public:
    ScreenManager(xcb_connection_t* conn, int defaultScreen);
    ~ScreenManager() = default;

    xcb_screen_t* getDefaultScreen() const { return m_defaultScreen; }
    const std::vector<xcb_screen_t*>& getAllScreens() const { return m_screens; }

private:
    xcb_screen_t* m_defaultScreen{nullptr};
    std::vector<xcb_screen_t*> m_screens;
};

} // namespace luma::platform::x11::core
