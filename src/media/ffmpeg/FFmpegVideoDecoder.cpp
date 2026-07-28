#include "FFmpegVideoDecoder.hpp"
#include <core/Logging.hpp>
#include <media/common/MediaTelemetry.hpp>
#include <chrono>

namespace luma::media {

FFmpegVideoDecoder::FFmpegVideoDecoder() {
    m_hwManager = std::make_shared<HardwareDecodeManager>();
    m_converter = std::make_shared<FFmpegFrameConverter>();
}

FFmpegVideoDecoder::~FFmpegVideoDecoder() {
    stop();
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
    }
}

bool FFmpegVideoDecoder::open(AVStream* stream) {
    if (!stream) return false;
    m_stream = stream;

    const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder) {
        spdlog::error("[MEDIA] Unsupported codec");
        return false;
    }

    m_codecCtx = avcodec_alloc_context3(decoder);
    if (!m_codecCtx) return false;

    avcodec_parameters_to_context(m_codecCtx, stream->codecpar);

    // Try initializing hardware decode
    if (m_hwDecodeEnabled && m_hwManager->init(decoder->id)) {
        m_codecCtx->hw_device_ctx = av_buffer_ref(m_hwManager->getDeviceContext());
        m_codecCtx->get_format = HardwareDecodeManager::getFormatCallback;
        m_codecCtx->opaque = m_hwManager.get();
    } else if (!m_hwDecodeEnabled) {
        spdlog::info("[MEDIA] Hardware decode disabled by fallback manager, using software decoder.");
    }

    if (avcodec_open2(m_codecCtx, decoder, nullptr) < 0) {
        spdlog::error("[MEDIA] Failed to open codec");
        return false;
    }

    return true;
}

void FFmpegVideoDecoder::start(std::shared_ptr<FFmpegPacketQueue> packetQueue, std::shared_ptr<FrameQueue> frameQueue) {
    stop();
    m_packetQueue = packetQueue;
    m_frameQueue = frameQueue;
    m_running = true;
    m_thread = std::thread(&FFmpegVideoDecoder::decodeLoop, this);
}

void FFmpegVideoDecoder::stop() {
    m_running = false;
    // We expect the queues to be aborted by the MediaSession so the loop unblocks
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void FFmpegVideoDecoder::flush() {
    if (m_codecCtx) {
        avcodec_flush_buffers(m_codecCtx);
    }
}

void FFmpegVideoDecoder::decodeLoop() {
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    double timeBase = av_q2d(m_stream->time_base);

    while (m_running) {
        if (!m_packetQueue->pop(packet)) {
            // Queue aborted (seek or stop)
            continue;
        }

        int ret = avcodec_send_packet(m_codecCtx, packet);
        av_packet_unref(packet);

        if (ret < 0) {
            spdlog::error("[MEDIA] Error sending packet to decoder");
            continue;
        }

        while (ret >= 0) {
            auto startDecode = std::chrono::high_resolution_clock::now();
            ret = avcodec_receive_frame(m_codecCtx, frame);
            auto endDecode = std::chrono::high_resolution_clock::now();
            
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                spdlog::error("[MEDIA] Error receiving frame from decoder");
                break;
            }

            double decodeTimeMs = std::chrono::duration<double, std::milli>(endDecode - startDecode).count();
            MediaTelemetry::getInstance().recordDecodeLatency(decodeTimeMs);

            AVFrame* finalFrame = frame;
            AVFrame* swFrame = nullptr;

            // Handle hardware frames (transfer data to CPU)
            if (frame->format == m_hwManager->getHardwarePixelFormat()) {
                swFrame = av_frame_alloc();
                
                auto startTransfer = std::chrono::high_resolution_clock::now();
                if (av_hwframe_transfer_data(swFrame, frame, 0) < 0) {
                    spdlog::error("[MEDIA] Failed to transfer hardware frame to CPU");
                    av_frame_free(&swFrame);
                    continue;
                }
                auto endTransfer = std::chrono::high_resolution_clock::now();
                double transferTimeMs = std::chrono::duration<double, std::milli>(endTransfer - startTransfer).count();
                // Add transfer time to decode latency for sw fallback
                MediaTelemetry::getInstance().recordDecodeLatency(transferTimeMs);
                
                swFrame->pts = frame->pts;
                swFrame->duration = frame->duration;
                finalFrame = swFrame;
            }

            // Convert and Push
            auto mediaFrame = m_converter->convert(finalFrame, timeBase);
            if (mediaFrame) {
                if (!m_frameQueue->push(mediaFrame)) {
                    // FrameQueue aborted
                }
            }

            if (swFrame) {
                av_frame_free(&swFrame);
            }
            av_frame_unref(frame);
        }
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
}

} // namespace luma::media
