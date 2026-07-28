#pragma once

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include "DatabaseManager.hpp"
#include "ThumbnailCache.hpp"
#include "FileWatcher.hpp"
#include <media/ffmpeg/FFmpegDemuxer.hpp>

namespace luma::library {

class LibraryScanner {
public:
    LibraryScanner(std::shared_ptr<DatabaseManager> db);
    ~LibraryScanner();

    void scanDirectory(const std::string& directoryPath);
    void startWatching(const std::string& directoryPath);

private:
    void handleFileEvent(const std::string& filePath, FileEvent event);
    bool isSupportedFormat(const std::string& extension);
    bool extractMetadata(const std::string& filePath, WallpaperRecord& record);

    std::shared_ptr<DatabaseManager> m_db;
    std::shared_ptr<ThumbnailCache> m_thumbnailCache;
    FileWatcher m_watcher;
};

} // namespace luma::library
