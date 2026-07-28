#pragma once
#include <xcb/xcb.h>
#include <atomic>
#include <thread>
#include <functional>

namespace luma::platform::x11::core {

class EventLoop {
public:
    using EventHandler = std::function<void(xcb_generic_event_t*)>;

    EventLoop(xcb_connection_t* conn);
    ~EventLoop();

    void start(EventHandler handler);
    void stop();

private:
    void loop();

    xcb_connection_t* m_conn;
    EventHandler m_handler;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    int m_eventFd{-1};
};

} // namespace luma::platform::x11::core
