#include "FileWatcher.hpp"
#include <core/Logging.hpp>
#include <sys/inotify.h>
#include <unistd.h>
#include <filesystem>
#include <iostream>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

namespace luma::library {

FileWatcher::FileWatcher() = default;

FileWatcher::~FileWatcher() {
    stop();
}

bool FileWatcher::start(const std::string& directoryPath, FileEventCallback callback) {
    if (m_running) return false;

    m_fd = inotify_init1(IN_NONBLOCK);
    if (m_fd < 0) {
        spdlog::error("[LIBRARY] inotify_init failed");
        return false;
    }

    m_callback = std::move(callback);
    m_basePath = directoryPath;
    
    if (!std::filesystem::exists(directoryPath)) {
        std::filesystem::create_directories(directoryPath);
    }
    
    addWatchRecursive(directoryPath);

    m_running = true;
    m_thread = std::thread(&FileWatcher::watchLoop, this);
    
    spdlog::info("[LIBRARY] Started FileWatcher on {}", directoryPath);
    return true;
}

void FileWatcher::stop() {
    if (!m_running) return;

    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }

    for (const auto& [wd, path] : m_watchDescriptors) {
        inotify_rm_watch(m_fd, wd);
    }
    m_watchDescriptors.clear();

    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
}

void FileWatcher::addWatchRecursive(const std::string& path) {
    int wd = inotify_add_watch(m_fd, path.c_str(), IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO);
    if (wd >= 0) {
        m_watchDescriptors[wd] = path;
    }

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_directory()) {
            addWatchRecursive(entry.path().string());
        }
    }
}

void FileWatcher::watchLoop() {
    char buffer[BUF_LEN];

    while (m_running) {
        int length = read(m_fd, buffer, BUF_LEN);  
        
        if (length < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        int i = 0;
        while (i < length) {
            struct inotify_event* event = (struct inotify_event*) &buffer[i];
            
            if (event->len) {
                auto it = m_watchDescriptors.find(event->wd);
                if (it != m_watchDescriptors.end()) {
                    std::string dirPath = it->second;
                    std::string fullPath = dirPath + "/" + event->name;
                    
                    FileEvent evType = FileEvent::Modified;
                    if (event->mask & IN_CREATE) evType = FileEvent::Created;
                    else if (event->mask & IN_DELETE) evType = FileEvent::Deleted;
                    else if (event->mask & IN_MOVED_TO) evType = FileEvent::Moved;
                    
                    if (event->mask & IN_ISDIR && (event->mask & IN_CREATE)) {
                        addWatchRecursive(fullPath);
                    }
                    
                    if (m_callback && (event->mask & (IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO))) {
                        m_callback(fullPath, evType);
                    }
                }
            }
            i += EVENT_SIZE + event->len;
        }
    }
}

} // namespace luma::library
