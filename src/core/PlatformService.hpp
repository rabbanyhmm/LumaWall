#pragma once
#include <memory>
#include <platform/common/IWallpaperSurface.hpp>
#include <platform/common/IMonitor.hpp>

namespace luma::core {

enum class PlatformType {
    Wayland,
    X11,
    Unknown
};

class PlatformService {
public:
    static PlatformType detectPlatform();
    
    virtual ~PlatformService() = default;
    
    virtual bool init() = 0;
    virtual void shutdown() = 0;
    virtual std::shared_ptr<luma::platform::IWallpaperSurface> createWallpaperSurface(std::shared_ptr<luma::platform::IMonitor> monitor) = 0;
    
};

} // namespace luma::core
