#include "FFmpegPacketQueue.hpp"

namespace luma::media {

FFmpegPacketQueue::FFmpegPacketQueue(size_t maxCapacity) : m_maxCapacity(maxCapacity) {
}

FFmpegPacketQueue::~FFmpegPacketQueue() {
    flush();
    abort();
}

bool FFmpegPacketQueue::push(AVPacket* pkt) {
    AVPacket* pktCopy = av_packet_alloc();
    av_packet_move_ref(pktCopy, pkt);

    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [this]() { return m_abortRequest || m_queue.size() < m_maxCapacity; });

    if (m_abortRequest) {
        av_packet_free(&pktCopy);
        return false;
    }

    m_queue.push(pktCopy);
    lock.unlock();
    m_cond.notify_one();
    return true;
}

bool FFmpegPacketQueue::pop(AVPacket* pkt) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [this]() { return m_abortRequest || !m_queue.empty(); });

    if (m_abortRequest) {
        return false;
    }

    AVPacket* frontPkt = m_queue.front();
    m_queue.pop();
    
    av_packet_move_ref(pkt, frontPkt);
    av_packet_free(&frontPkt);

    lock.unlock();
    m_cond.notify_one(); // Notify pushers that space is available
    return true;
}

void FFmpegPacketQueue::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_queue.empty()) {
        AVPacket* pkt = m_queue.front();
        m_queue.pop();
        av_packet_free(&pkt);
    }
    m_cond.notify_all();
}

void FFmpegPacketQueue::abort() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_abortRequest = true;
    m_cond.notify_all();
}

void FFmpegPacketQueue::start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_abortRequest = false;
}

size_t FFmpegPacketQueue::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

bool FFmpegPacketQueue::isFull() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size() >= m_maxCapacity;
}

} // namespace luma::media
