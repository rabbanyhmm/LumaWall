#include "FFmpegAudioDecoder.hpp"
#include <core/Logging.hpp>

namespace luma::media {

FFmpegAudioDecoder::~FFmpegAudioDecoder() {
    stop();
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
    }
}

bool FFmpegAudioDecoder::open(AVStream* stream) {
    if (!stream) return false;
    m_stream = stream;

    const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder) return false;

    m_codecCtx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(m_codecCtx, stream->codecpar);

    if (avcodec_open2(m_codecCtx, decoder, nullptr) < 0) {
        return false;
    }
    return true;
}

void FFmpegAudioDecoder::start(std::shared_ptr<FFmpegPacketQueue> packetQueue) {
    stop();
    m_packetQueue = packetQueue;
    m_running = true;
    m_thread = std::thread(&FFmpegAudioDecoder::decodeLoop, this);
}

void FFmpegAudioDecoder::stop() {
    m_running = false;
    if (m_thread.joinable()) m_thread.join();
}

void FFmpegAudioDecoder::flush() {
    if (m_codecCtx) avcodec_flush_buffers(m_codecCtx);
}

void FFmpegAudioDecoder::decodeLoop() {
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    while (m_running) {
        if (!m_packetQueue->pop(packet)) {
            continue;
        }

        // Just consume and ignore for now (skeleton)
        int ret = avcodec_send_packet(m_codecCtx, packet);
        av_packet_unref(packet);

        if (ret >= 0) {
            while (avcodec_receive_frame(m_codecCtx, frame) >= 0) {
                av_frame_unref(frame);
            }
        }
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
}

} // namespace luma::media
