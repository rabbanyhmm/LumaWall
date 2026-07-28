#pragma once

#include <thread>
#include <atomic>

namespace luma::platform::wayland::core {

class DisplayConnection;

class EventLoop {
public:
    EventLoop(DisplayConnection& display);
    ~EventLoop();

    void start();
    void stop();

private:
    void run();

    DisplayConnection& m_display;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    int m_eventFd{-1};
};

} // namespace luma::platform::wayland::core
