#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include "FFmpegPacketQueue.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace luma::media {

class FFmpegAudioDecoder {
public:
    FFmpegAudioDecoder() = default;
    ~FFmpegAudioDecoder();

    bool open(AVStream* stream);
    void start(std::shared_ptr<FFmpegPacketQueue> packetQueue);
    void stop();
    void flush();

private:
    void decodeLoop();

    AVCodecContext* m_codecCtx{nullptr};
    AVStream* m_stream{nullptr};
    std::shared_ptr<FFmpegPacketQueue> m_packetQueue;
    
    std::thread m_thread;
    std::atomic<bool> m_running{false};
};

} // namespace luma::media
