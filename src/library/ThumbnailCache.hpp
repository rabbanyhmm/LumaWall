#pragma once

#include <string>
#include <optional>

namespace luma::library {

class ThumbnailCache {
public:
    ThumbnailCache();
    ~ThumbnailCache();

    // Returns the path to the cached thumbnail, generating it if it doesn't exist
    std::optional<std::string> getThumbnail(const std::string& videoPath);
    
    // Explicitly generate a thumbnail
    bool generateThumbnail(const std::string& videoPath, const std::string& outPath);

private:
    std::string m_cacheDir;
    
    std::string computeCacheKey(const std::string& videoPath) const;
};

} // namespace luma::library
