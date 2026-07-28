#pragma once
#include <chrono>

namespace luma::media {

class Clock {
public:
    Clock() = default;

    void start();
    void pause();
    void stop();
    void seek(double timestamp);
    void setPlaybackSpeed(float speed);

    double getCurrentTime() const;
    bool isPlaying() const { return m_playing; }

private:
    bool m_playing{false};
    float m_speed{1.0f};
    double m_currentTime{0.0};
    std::chrono::high_resolution_clock::time_point m_lastUpdate;
};

} // namespace luma::media
