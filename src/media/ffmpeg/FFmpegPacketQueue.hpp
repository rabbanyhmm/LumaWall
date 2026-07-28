#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace luma::media {

class FFmpegPacketQueue {
public:
    explicit FFmpegPacketQueue(size_t maxCapacity = 100);
    ~FFmpegPacketQueue();

    // Push packet into queue. Blocks if full. Returns false if aborted.
    bool push(AVPacket* pkt);

    // Pop packet from queue. Blocks if empty. Returns false if aborted.
    bool pop(AVPacket* pkt);

    void flush();
    void abort();
    void start();

    size_t size() const;
    bool isFull() const;

private:
    size_t m_maxCapacity;
    bool m_abortRequest{false};
    
    mutable std::mutex m_mutex;
    std::condition_variable m_cond;
    std::queue<AVPacket*> m_queue;
};

} // namespace luma::media
