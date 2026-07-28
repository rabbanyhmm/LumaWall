#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include "FFmpegPacketQueue.hpp"

extern "C" {
#include <libavformat/avformat.h>
}

namespace luma::media {

class FFmpegDemuxer {
public:
    FFmpegDemuxer();
    ~FFmpegDemuxer();

    bool open(const std::string& uri);
    void start(std::shared_ptr<FFmpegPacketQueue> packetQueue);
    void stop();
    void seek(double timestamp);
    void setLooping(bool loop) { m_looping = loop; }

    int getVideoStreamIndex() const { return m_videoStreamIndex; }
    AVStream* getVideoStream() const;
    AVFormatContext* getFormatContext() const { return m_formatCtx; }

private:
    void demuxLoop();

    AVFormatContext* m_formatCtx{nullptr};
    int m_videoStreamIndex{-1};

    std::shared_ptr<FFmpegPacketQueue> m_packetQueue;
    
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_looping{false};

    std::atomic<bool> m_seekRequest{false};
    std::atomic<double> m_seekTimestamp{0.0};
};

} // namespace luma::media
