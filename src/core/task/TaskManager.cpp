#include "TaskManager.hpp"
#include <core/Logging.hpp>

namespace luma::core::task {

TaskManager& TaskManager::instance() {
    static TaskManager instance(std::thread::hardware_concurrency());
    return instance;
}

TaskManager::TaskManager(size_t numThreads) : m_stop(false) {
    start(numThreads);
}

TaskManager::~TaskManager() {
    stop();
}

void TaskManager::start(size_t numThreads) {
    if (!m_workers.empty()) {
        return;
    }
    
    m_stop = false;
    for (size_t i = 0; i < numThreads; ++i) {
        m_workers.emplace_back([this] {
            workerThread();
        });
    }
    spdlog::info("[TASK] Started TaskManager with {} threads", numThreads);
}

void TaskManager::stop() {
    if (m_stop) return;
    
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop = true;
    }
    
    m_condition.notify_all();
    
    for (std::thread& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_workers.clear();
    spdlog::info("[TASK] Stopped TaskManager");
}

void TaskManager::workerThread() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
            
            if (m_stop && m_tasks.empty()) {
                return;
            }
            
            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        
        try {
            task();
        } catch (const std::exception& e) {
            spdlog::error("[TASK] Exception in background task: {}", e.what());
        } catch (...) {
            spdlog::error("[TASK] Unknown exception in background task");
        }
    }
}

} // namespace luma::core::task
