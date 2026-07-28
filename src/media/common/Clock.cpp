#include "Clock.hpp"

namespace luma::media {

void Clock::start() {
    if (!m_playing) {
        m_playing = true;
        m_lastUpdate = std::chrono::high_resolution_clock::now();
    }
}

void Clock::pause() {
    if (m_playing) {
        // Update current time before pausing
        getCurrentTime();
        m_playing = false;
    }
}

void Clock::stop() {
    m_playing = false;
    m_currentTime = 0.0;
}

void Clock::seek(double timestamp) {
    m_currentTime = timestamp;
    if (m_playing) {
        m_lastUpdate = std::chrono::high_resolution_clock::now();
    }
}

void Clock::setPlaybackSpeed(float speed) {
    // Update time before changing speed
    if (m_playing) {
        getCurrentTime();
    }
    m_speed = speed;
    if (m_playing) {
        m_lastUpdate = std::chrono::high_resolution_clock::now();
    }
}

double Clock::getCurrentTime() const {
    if (m_playing) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = now - m_lastUpdate;
        const_cast<Clock*>(this)->m_currentTime += diff.count() * m_speed;
        const_cast<Clock*>(this)->m_lastUpdate = now;
    }
    return m_currentTime;
}

} // namespace luma::media
