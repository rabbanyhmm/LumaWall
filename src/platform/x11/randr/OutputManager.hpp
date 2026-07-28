#pragma once
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <core/events/EventBus.hpp>
#include <memory>

namespace luma::platform::x11::randr {

class OutputManager {
public:
    OutputManager(xcb_connection_t* conn, xcb_screen_t* screen, std::shared_ptr<luma::core::events::EventBus> eventBus);
    ~OutputManager() = default;

    bool init();
    void handleRandrEvent(xcb_generic_event_t* event);

private:
    void queryOutputs();

    xcb_connection_t* m_conn;
    xcb_screen_t* m_screen;
    std::shared_ptr<luma::core::events::EventBus> m_eventBus;
    uint8_t m_randrEventBase{0};
};

} // namespace luma::platform::x11::randr
