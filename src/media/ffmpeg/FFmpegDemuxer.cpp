#include "FFmpegDemuxer.hpp"
#include <core/Logging.hpp>

namespace luma::media {

FFmpegDemuxer::FFmpegDemuxer() {
}

FFmpegDemuxer::~FFmpegDemuxer() {
    stop();
    if (m_formatCtx) {
        avformat_close_input(&m_formatCtx);
    }
}

bool FFmpegDemuxer::open(const std::string& uri) {
    if (m_formatCtx) {
        avformat_close_input(&m_formatCtx);
    }

    if (avformat_open_input(&m_formatCtx, uri.c_str(), nullptr, nullptr) < 0) {
        spdlog::error("[MEDIA] Could not open source: {}", uri);
        return false;
    }

    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        spdlog::error("[MEDIA] Could not find stream information");
        return false;
    }

    m_videoStreamIndex = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_videoStreamIndex < 0) {
        spdlog::warn("[MEDIA] Could not find video stream in source: {}", uri);
    }

    return true;
}

AVStream* FFmpegDemuxer::getVideoStream() const {
    if (m_videoStreamIndex >= 0 && m_formatCtx) {
        return m_formatCtx->streams[m_videoStreamIndex];
    }
    return nullptr;
}

void FFmpegDemuxer::start(std::shared_ptr<FFmpegPacketQueue> packetQueue) {
    stop();
    m_packetQueue = packetQueue;
    m_running = true;
    m_thread = std::thread(&FFmpegDemuxer::demuxLoop, this);
}

void FFmpegDemuxer::stop() {
    m_running = false;
    if (m_packetQueue) {
        m_packetQueue->abort();
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void FFmpegDemuxer::seek(double timestamp) {
    m_seekTimestamp = timestamp;
    m_seekRequest = true;
    if (m_packetQueue) {
        m_packetQueue->abort(); // Abort any blocking pushes to let the loop process the seek
    }
}

void FFmpegDemuxer::demuxLoop() {
    AVPacket* packet = av_packet_alloc();

    while (m_running) {
        if (m_seekRequest) {
            double timestamp = m_seekTimestamp.exchange(0.0);
            m_seekRequest = false;

            if (m_videoStreamIndex >= 0) {
                AVStream* stream = m_formatCtx->streams[m_videoStreamIndex];
                int64_t seekTarget = timestamp / av_q2d(stream->time_base);
                av_seek_frame(m_formatCtx, m_videoStreamIndex, seekTarget, AVSEEK_FLAG_BACKWARD);
            }

            m_packetQueue->flush();
            m_packetQueue->start();
        }

        int ret = av_read_frame(m_formatCtx, packet);
        
        if (ret == AVERROR_EOF) {
            if (m_looping) {
                // Internal loop: Seek to beginning WITHOUT flushing the packet queue!
                // This ensures all remaining packets in the queue are decoded.
                if (m_videoStreamIndex >= 0) {
                    av_seek_frame(m_formatCtx, m_videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
                }
                continue;
            } else {
                // Wait/sleep a bit until stop/seek is requested
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
        } else if (ret < 0) {
            spdlog::error("[MEDIA] Error reading frame");
            break;
        }

        // Push packet
        if (packet->stream_index == m_videoStreamIndex) {
            if (!m_packetQueue->push(packet)) {
                // Push aborted (e.g. seeking or stopping)
                av_packet_unref(packet);
                continue;
            }
        } else {
            // Audio or other stream, ignore for now
        }

        av_packet_unref(packet);
    }

    av_packet_free(&packet);
}

} // namespace luma::media
