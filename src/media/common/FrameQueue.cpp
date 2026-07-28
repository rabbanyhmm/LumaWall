#include "FrameQueue.hpp"

namespace luma::media {

FrameQueue::FrameQueue(size_t maxCapacity) : m_maxCapacity(maxCapacity) {
}

bool FrameQueue::push(std::shared_ptr<Frame> frame) {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    m_cv.wait(lock, [this]() { return m_abortRequest || m_queue.size() < m_maxCapacity; });

    if (m_abortRequest) return false;

    m_queue.push(std::move(frame));
    lock.unlock();
    m_cv.notify_one();
    return true;
}

std::shared_ptr<Frame> FrameQueue::pop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty() || m_abortRequest) {
        return nullptr;
    }
    
    auto frame = m_queue.front();
    m_queue.pop();
    m_cv.notify_all(); // Notify decode thread that space is available
    return frame;
}

double FrameQueue::peekNextTimestamp() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty()) return -1.0;
    return m_queue.front()->timestamp;
}

void FrameQueue::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::queue<std::shared_ptr<Frame>> empty;
    std::swap(m_queue, empty);
    m_cv.notify_all();
}

void FrameQueue::abort() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_abortRequest = true;
    m_cv.notify_all();
}

void FrameQueue::start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_abortRequest = false;
}

size_t FrameQueue::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

bool FrameQueue::isFull() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size() >= m_maxCapacity;
}

bool FrameQueue::isEmpty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

void FrameQueue::setMaxCapacity(size_t capacity) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxCapacity = capacity;
    m_cv.notify_all();
}

} // namespace luma::media
