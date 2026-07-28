#include "PackageFormat.hpp"
#include <core/Logging.hpp>
#include <filesystem>
#include <cstdlib>

namespace luma::library {

PackageFormat::PackageFormat(std::shared_ptr<DatabaseManager> db)
    : m_db(std::move(db)) {
    
    // Default storage directory
    const char* home = std::getenv("HOME");
    m_storageDir = home ? std::string(home) + "/.local/share/lumawall/wallpapers" : "/tmp/lumawall/wallpapers";
    
    std::filesystem::create_directories(m_storageDir);
}

PackageFormat::~PackageFormat() = default;

bool PackageFormat::extractZip(const std::string& zipPath, const std::string& destDir) {
    std::filesystem::create_directories(destDir);
    
    // MVP implementation using standard system unzip
    std::string command = "unzip -o -q \"" + zipPath + "\" -d \"" + destDir + "\"";
    int ret = std::system(command.c_str());
    
    if (ret != 0) {
        spdlog::error("[LIBRARY] Failed to extract package: {}", zipPath);
        return false;
    }
    
    return true;
}

bool PackageFormat::parseManifestAndRegister(const std::string& destDir, int& outId) {
    // In a full implementation, we would parse manifest.json here.
    // For MVP, we'll scan the directory for the first video file.
    
    for (const auto& entry : std::filesystem::directory_iterator(destDir)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            if (ext == ".mp4" || ext == ".webm" || ext == ".mkv") {
                
                WallpaperRecord record;
                record.path = entry.path().string();
                record.name = std::filesystem::path(destDir).filename().string();
                
                if (m_db->addWallpaper(record)) {
                    outId = 0; // Ideally we'd get the ID from addWallpaper or a SELECT
                    return true;
                }
            }
        }
    }
    
    spdlog::error("[LIBRARY] No valid media found in package: {}", destDir);
    return false;
}

int PackageFormat::importPackage(const std::string& packagePath) {
    if (!std::filesystem::exists(packagePath)) {
        spdlog::error("[LIBRARY] Package does not exist: {}", packagePath);
        return -1;
    }

    std::string packageName = std::filesystem::path(packagePath).stem().string();
    std::string destDir = m_storageDir + "/" + packageName;

    if (!extractZip(packagePath, destDir)) {
        return -1;
    }

    int newId = -1;
    if (parseManifestAndRegister(destDir, newId)) {
        spdlog::info("[LIBRARY] Successfully imported package: {}", packageName);
        return newId;
    }

    return -1;
}

} // namespace luma::library
