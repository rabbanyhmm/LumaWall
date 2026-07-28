#include "EventLoop.hpp"
#include "DisplayConnection.hpp"
#include <sys/eventfd.h>
#include <unistd.h>
#include <poll.h>
#include <iostream>

namespace luma::platform::wayland::core {

EventLoop::EventLoop(DisplayConnection& display) : m_display(display) {
    m_eventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
}

EventLoop::~EventLoop() {
    stop();
    if (m_eventFd != -1) {
        close(m_eventFd);
    }
}

void EventLoop::start() {
    if (!m_running.exchange(true)) {
        m_thread = std::thread(&EventLoop::run, this);
    }
}

void EventLoop::stop() {
    if (m_running.exchange(false)) {
        uint64_t val = 1;
        if (write(m_eventFd, &val, sizeof(val)) < 0) {
            // handle error if needed
        }
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }
}

void EventLoop::run() {
    struct pollfd fds[2];
    fds[0].fd = m_display.getFd();
    fds[0].events = POLLIN;
    fds[1].fd = m_eventFd;
    fds[1].events = POLLIN;

    while (m_running) {
        while (wl_display_prepare_read(m_display.get()) != 0) {
            wl_display_dispatch_pending(m_display.get());
        }
        
        m_display.flush();

        int ret = poll(fds, 2, -1);
        if (ret < 0) {
            wl_display_cancel_read(m_display.get());
            break;
        }
        
        if (fds[1].revents & POLLIN) {
            wl_display_cancel_read(m_display.get());
            break;
        }

        if (fds[0].revents & POLLIN) {
            wl_display_read_events(m_display.get());
            wl_display_dispatch_pending(m_display.get());
        } else {
            wl_display_cancel_read(m_display.get());
        }
    }
}

} // namespace luma::platform::wayland::core
