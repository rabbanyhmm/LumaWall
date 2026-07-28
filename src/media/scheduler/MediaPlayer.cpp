#include "MediaPlayer.hpp"
#include <media/image/ImageDecoder.hpp>
#include <media/ffmpeg/FFmpegMediaSession.hpp>
#include <core/Logging.hpp>

namespace luma::media {

MediaPlayer::MediaPlayer() {
    m_frameQueue = std::make_shared<FrameQueue>(10);
}

MediaPlayer::~MediaPlayer() {
    stop();
}

void MediaPlayer::load(std::shared_ptr<IMediaSource> source) {
    stop();
    m_source = source;
    
    if (m_source) {
        if (m_source->getType() == MediaType::Image) {
            m_decoder = std::make_shared<ImageDecoder>(m_source, m_frameQueue);
        } else if (m_source->getType() == MediaType::Video) {
            m_decoder = std::make_shared<FFmpegMediaSession>(m_source, m_frameQueue);
            if (!m_supportedFormats.empty()) {
                m_decoder->setSupportedFormats(m_supportedFormats);
            }
        } else {
            spdlog::error("[MEDIA] Unsupported media type for now");
            return;
        }

        if (m_decoder->init()) {
            m_decoder->setHardwareDecodeEnabled(m_hwDecodeEnabled);
            spdlog::info("[MEDIA] Loaded source: {}", m_source->getURI());
        } else {
            m_decoder.reset();
        }
    }
}

void MediaPlayer::play() {
    if (m_decoder) {
        m_decoder->start();
        m_clock.start();
    }
}

void MediaPlayer::pause() {
    if (m_decoder) {
        m_decoder->pause();
        m_clock.pause();
    }
}

void MediaPlayer::stop() {
    if (m_decoder) {
        m_decoder->stop();
    }
    m_clock.stop();
    m_frameQueue->flush();
    m_currentFrame.reset();
}

void MediaPlayer::seek(double timestamp) {
    if (m_decoder) {
        m_decoder->seek(timestamp);
    }
    m_clock.seek(timestamp);
    m_frameQueue->flush();
}

void MediaPlayer::setPlaybackSpeed(float speed) {
    m_clock.setPlaybackSpeed(speed);
}

void MediaPlayer::setHardwareDecodeEnabled(bool enabled) {
    m_hwDecodeEnabled = enabled;
    if (m_decoder) {
        m_decoder->setHardwareDecodeEnabled(enabled);
    }
}

void MediaPlayer::setSupportedFormats(const std::vector<PixelFormat>& formats) {
    m_supportedFormats = formats;
    if (m_decoder) {
        m_decoder->setSupportedFormats(formats);
    }
}

MediaCompatibilityReport MediaPlayer::getReport() const {
    if (m_decoder) {
        return m_decoder->getReport();
    }
    return MediaCompatibilityReport{};
}

void MediaPlayer::setLooping(bool loop) {
    m_looping = loop;
}

std::shared_ptr<Frame> MediaPlayer::getNextFrame() {
    if (!m_decoder) return nullptr;

    double currentTime = m_clock.getCurrentTime();

    // Check if we need a new frame based on the clock
    // For static images, we just take the first frame and hold it.
    if (!m_currentFrame && !m_frameQueue->isEmpty()) {
        m_currentFrame = m_frameQueue->pop();
    } else if (m_currentFrame && m_currentFrame->duration > 0.0) {
        double nextFrameTime = m_frameQueue->peekNextTimestamp();
        
        // If the clock has passed the next frame's timestamp, pop it.
        // Wait, if nextFrameTime == -1.0, queue is empty.
        if (nextFrameTime >= 0.0 && nextFrameTime < m_currentFrame->timestamp - 0.5) {
            // Demuxer looped internally. Wait for the current frame to finish.
            if (currentTime >= m_currentFrame->timestamp + m_currentFrame->duration) {
                m_clock.seek(nextFrameTime); // Resync clock!
                m_currentFrame = m_frameQueue->pop();
            }
        } else if (nextFrameTime >= 0.0 && currentTime >= nextFrameTime) {
            m_currentFrame = m_frameQueue->pop();
        } else if (currentTime >= m_currentFrame->timestamp + m_currentFrame->duration) {
            // Reached end of current frame but next frame not ready or doesn't exist
            if (m_looping && m_source->getDuration() > 0.0 && currentTime >= m_source->getDuration()) {
                seek(0.0);
            }
        }
    }
    
    return m_currentFrame;
}

} // namespace luma::media
