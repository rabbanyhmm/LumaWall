#pragma once

#include "Events.hpp"
#include <queue>
#include <mutex>
#include <optional>
#include <vector>

namespace luma::core::events {

class EventBus {
public:
    void pushEvent(PlatformEvent event) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(event));
    }

    std::optional<PlatformEvent> pollEvent() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return std::nullopt;
        }
        auto event = std::move(m_queue.front());
        m_queue.pop();
        return event;
    }

    std::vector<PlatformEvent> pollAllEvents() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<PlatformEvent> events;
        events.reserve(m_queue.size());
        while (!m_queue.empty()) {
            events.push_back(std::move(m_queue.front()));
            m_queue.pop();
        }
        return events;
    }

private:
    std::queue<PlatformEvent> m_queue;
    std::mutex m_mutex;
};

} // namespace luma::core::events
