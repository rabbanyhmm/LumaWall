#include "LibraryScanner.hpp"
#include <core/Logging.hpp>
#include <core/task/TaskManager.hpp>
#include <filesystem>
#include <algorithm>

namespace luma::library {

LibraryScanner::LibraryScanner(std::shared_ptr<DatabaseManager> db)
    : m_db(std::move(db)) {
    m_thumbnailCache = std::make_shared<ThumbnailCache>();
}

LibraryScanner::~LibraryScanner() {
    m_watcher.stop();
}

bool LibraryScanner::isSupportedFormat(const std::string& extension) {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".mp4" || ext == ".webm" || ext == ".mkv" || ext == ".mov";
}

bool LibraryScanner::extractMetadata(const std::string& filePath, WallpaperRecord& record) {
    luma::media::FFmpegDemuxer demuxer;
    if (!demuxer.open(filePath)) {
        return false;
    }

    auto* stream = demuxer.getVideoStream();
    if (!stream) {
        return false;
    }

    auto* ctx = demuxer.getFormatContext();
    if (ctx->duration != AV_NOPTS_VALUE) {
        record.duration = static_cast<double>(ctx->duration) / AV_TIME_BASE;
    }

    if (stream->codecpar) {
        record.width = stream->codecpar->width;
        record.height = stream->codecpar->height;
        // Basic guess of format string
        record.format = avcodec_get_name(stream->codecpar->codec_id);
        if (record.format.empty()) record.format = "Unknown";
    }

    return true;
}

void LibraryScanner::scanDirectory(const std::string& directoryPath) {
    if (!m_db) return;

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                const auto& path = entry.path();
                if (isSupportedFormat(path.extension().string())) {
                    std::string filePath = path.string();
                    std::string fileName = path.filename().string();

                    WallpaperRecord record;
                    record.path = filePath;
                    record.name = fileName;

                    if (extractMetadata(filePath, record)) {
                        if (m_db->addWallpaper(record)) {
                            spdlog::info("[LIBRARY] Added wallpaper: {}", fileName);
                            
                            // Generate thumbnail in background
                            std::string pathCopy = filePath;
                            auto cacheCopy = m_thumbnailCache;
                            luma::core::task::TaskManager::instance().enqueue([pathCopy, cacheCopy]() {
                                cacheCopy->getThumbnail(pathCopy);
                            });
                        }
                    }
                }
            }
        }
} catch (const std::exception& e) {
        spdlog::error("[LIBRARY] Scan error: {}", e.what());
    }
}

void LibraryScanner::startWatching(const std::string& directoryPath) {
    scanDirectory(directoryPath); // Initial scan
    m_watcher.start(directoryPath, [this](const std::string& path, FileEvent event) {
        luma::core::task::TaskManager::instance().enqueue([this, path, event]() {
            handleFileEvent(path, event);
        });
    });
}

void LibraryScanner::handleFileEvent(const std::string& filePath, FileEvent event) {
    std::string fileName = std::filesystem::path(filePath).filename().string();
    std::string ext = std::filesystem::path(filePath).extension().string();

    if (!isSupportedFormat(ext) && event != FileEvent::Deleted) return;

    if (event == FileEvent::Created || event == FileEvent::Moved) {
        WallpaperRecord record;
        record.path = filePath;
        record.name = fileName;
        if (extractMetadata(filePath, record)) {
            if (m_db->addWallpaper(record)) {
                spdlog::info("[LIBRARY-WATCHER] Added new wallpaper: {}", fileName);
                m_thumbnailCache->getThumbnail(filePath);
            }
        }
    } else if (event == FileEvent::Deleted) {
        // Here we'd remove from DB (if implemented in DatabaseManager)
        // m_db->removeWallpaper(filePath);
        spdlog::info("[LIBRARY-WATCHER] Removed wallpaper: {}", fileName);
    }
}

} // namespace luma::library
