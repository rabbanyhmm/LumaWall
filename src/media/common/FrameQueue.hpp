#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <media/common/Frame.hpp>

namespace luma::media {

class FrameQueue {
public:
    explicit FrameQueue(size_t maxCapacity = 10);
    ~FrameQueue() = default;

    // Push frame into queue. Blocks if full unless drop policy is applied.
    bool push(std::shared_ptr<Frame> frame);

    // Pop frame from queue. Non-blocking (returns nullptr if empty).
    std::shared_ptr<Frame> pop();
    
    // Peek at next frame timestamp without popping
    double peekNextTimestamp() const;

    void flush();
    void abort();
    void start();
    
    size_t size() const;
    bool isFull() const;
    bool isEmpty() const;

    void setMaxCapacity(size_t capacity);

private:
    size_t m_maxCapacity;
    bool m_abortRequest{false};
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<std::shared_ptr<Frame>> m_queue;
};

} // namespace luma::media
