#pragma once

#include <functional>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <memory>
#include <type_traits>

namespace luma::core::task {

class TaskManager {
public:
    static TaskManager& instance();

    TaskManager(size_t numThreads = std::thread::hardware_concurrency());
    ~TaskManager();

    // Prevent copying
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

    void start(size_t numThreads = std::thread::hardware_concurrency());
    void stop();

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type>;

private:
    void workerThread();

    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_stop;
};

// Template implementation
template<class F, class... Args>
auto TaskManager::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::invoke_result<F, Args...>::type> 
{
    using return_type = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        if (m_stop) {
            throw std::runtime_error("enqueue on stopped TaskManager");
        }
        m_tasks.emplace([task]() { (*task)(); });
    }
    m_condition.notify_one();
    return res;
}

} // namespace luma::core::task
