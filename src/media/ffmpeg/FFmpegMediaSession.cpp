#include "FFmpegMediaSession.hpp"
#include <core/Logging.hpp>

namespace luma::media {

FFmpegMediaSession::FFmpegMediaSession(std::shared_ptr<IMediaSource> source, std::shared_ptr<FrameQueue> frameQueue)
    : m_source(source), m_frameQueue(frameQueue) {
    m_videoPacketQueue = std::make_shared<FFmpegPacketQueue>(200);
    m_audioPacketQueue = std::make_shared<FFmpegPacketQueue>(500);

    m_demuxer = std::make_unique<FFmpegDemuxer>();
    m_videoDecoder = std::make_unique<FFmpegVideoDecoder>();
    m_audioDecoder = std::make_unique<FFmpegAudioDecoder>();
}

FFmpegMediaSession::~FFmpegMediaSession() {
    stop();
}

bool FFmpegMediaSession::init() {
    if (!m_demuxer->open(m_source->getURI())) {
        return false;
    }

    if (m_demuxer->getVideoStream()) {
        if (!m_videoDecoder->open(m_demuxer->getVideoStream())) {
            spdlog::warn("[MEDIA] Failed to open video decoder");
        }
    }

    // Audio stream initialization would go here (skipped for now as per skeleton)

    return true;
}

void FFmpegMediaSession::start() {
    m_frameQueue->start();
    m_videoPacketQueue->start();
    m_audioPacketQueue->start();

    m_videoDecoder->start(m_videoPacketQueue, m_frameQueue);
    // m_audioDecoder->start(m_audioPacketQueue);
    
    // Native demuxer looping provides zero-delay seamless playback
    m_demuxer->setLooping(true);
    m_demuxer->start(m_videoPacketQueue); // Simplified for single queue focus
}

void FFmpegMediaSession::pause() {
    // Decoding just pauses when frame queue fills up (back-pressure)
}

void FFmpegMediaSession::stop() {
    // Abort queues to unblock threads
    m_frameQueue->abort();
    m_videoPacketQueue->abort();
    m_audioPacketQueue->abort();

    m_demuxer->stop();
    m_videoDecoder->stop();
    m_audioDecoder->stop();
}

void FFmpegMediaSession::seek(double timestamp) {
    m_frameQueue->abort();
    m_videoPacketQueue->abort();
    m_audioPacketQueue->abort();
    
    m_videoDecoder->flush();
    m_audioDecoder->flush();

    m_demuxer->seek(timestamp);
    
    m_frameQueue->flush();
    m_videoPacketQueue->flush();
    m_audioPacketQueue->flush();

    m_frameQueue->start();
    m_videoPacketQueue->start();
    m_audioPacketQueue->start();
}

void FFmpegMediaSession::setSupportedFormats(const std::vector<PixelFormat>& formats) {
    m_videoDecoder->setSupportedFormats(formats);
}

void FFmpegMediaSession::setHardwareDecodeEnabled(bool enabled) {
    m_videoDecoder->setHardwareDecodeEnabled(enabled);
}

MediaCompatibilityReport FFmpegMediaSession::getReport() const {
    MediaCompatibilityReport report;
    report.codec = m_videoDecoder ? m_videoDecoder->getCodecName() : "Unknown";
    report.width = m_videoDecoder ? m_videoDecoder->getWidth() : 0;
    report.height = m_videoDecoder ? m_videoDecoder->getHeight() : 0;
    report.decoderBackend = "FFmpeg";
    if (m_videoDecoder && m_videoDecoder->isHardwareAccelerated()) {
        report.decoderBackend = "FFmpeg / VA-API";
        report.hardwareDecodingActive = true;
        // Zero-copy assumes DMABUF extraction succeeded
        report.zeroCopyActive = true; 
        report.uploadPath = "DMABUF -> DRM Modifier -> VkImage";
    } else {
        report.hardwareDecodingActive = false;
        report.zeroCopyActive = false;
        report.uploadPath = "CPU -> Staging Buffer -> VkImage";
    }
    
    return report;
}

void FFmpegMediaSession::decodeNextFrame() {
    // No-op for FFmpegMediaSession as decoding happens autonomously in background threads
}

} // namespace luma::media
