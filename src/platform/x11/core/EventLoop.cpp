#include "EventLoop.hpp"
#include <core/Logging.hpp>
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <cstdlib>

namespace luma::platform::x11::core {

EventLoop::EventLoop(xcb_connection_t* conn) : m_conn(conn) {
    m_eventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
}

EventLoop::~EventLoop() {
    stop();
    if (m_eventFd >= 0) {
        close(m_eventFd);
    }
}

void EventLoop::start(EventHandler handler) {
    if (m_running) return;
    
    m_handler = std::move(handler);
    m_running = true;
    m_thread = std::thread(&EventLoop::loop, this);
}

void EventLoop::stop() {
    if (!m_running) return;
    m_running = false;
    
    if (m_eventFd >= 0) {
        uint64_t val = 1;
        auto res = write(m_eventFd, &val, sizeof(val));
        (void)res;
    }
    
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void EventLoop::loop() {
    int xcbFd = xcb_get_file_descriptor(m_conn);
    if (xcbFd < 0 || m_eventFd < 0) {
        spdlog::error("[X11] Failed to get file descriptors for event loop");
        return;
    }

    struct pollfd fds[2] = {
        { xcbFd, POLLIN, 0 },
        { m_eventFd, POLLIN, 0 }
    };

    while (m_running) {
        xcb_generic_event_t* event;
        while (m_running && (event = xcb_poll_for_event(m_conn))) {
            if (m_handler) {
                m_handler(event);
            }
            free(event);
        }
        
        if (!m_running) break;
        
        xcb_flush(m_conn);

        if (poll(fds, 2, -1) < 0) {
            break;
        }

        if (fds[1].revents & POLLIN) {
            uint64_t val;
            auto res = read(m_eventFd, &val, sizeof(val));
            (void)res;
        }
    }
}

} // namespace luma::platform::x11::core
