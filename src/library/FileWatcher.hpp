#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <map>
#include <vector>

namespace luma::library {

enum class FileEvent {
    Created,
    Deleted,
    Modified,
    Moved
};

using FileEventCallback = std::function<void(const std::string&, FileEvent)>;

class FileWatcher {
public:
    FileWatcher();
    ~FileWatcher();

    bool start(const std::string& directoryPath, FileEventCallback callback);
    void stop();

private:
    void watchLoop();
    void addWatchRecursive(const std::string& path);

    int m_fd = -1;
    std::map<int, std::string> m_watchDescriptors; // wd -> path
    std::string m_basePath;
    
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    FileEventCallback m_callback;
};

} // namespace luma::library
