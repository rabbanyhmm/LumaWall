#pragma once

#include <memory>
#include <string>
#include <vector>
#include <core/events/EventBus.hpp>
#include <platform/common/IWallpaperSurface.hpp>
#include <platform/common/IMonitor.hpp>

namespace luma::platform {

struct BackendCapabilities {
    bool supportsLayerShell{false};
    bool supportsWallpaperSurface{false};
    bool supportsPerMonitor{false};
    bool supportsWorkspaceEvents{false};
    bool supportsHardwareZeroCopy{false};
};

class IMonitor;
class IWorkspaceManager;
class IDisplayManager;

class IPlatformBackend {
public:
    virtual ~IPlatformBackend() = default;

    virtual BackendCapabilities getCapabilities() const noexcept = 0;
    
    // Initialize the backend and wire up the event bus
    virtual bool init(std::shared_ptr<core::events::EventBus> eventBus) = 0;
    
    // Process pending platform events without blocking
    virtual void pumpEvents() = 0;

    virtual std::shared_ptr<IDisplayManager> getDisplayManager() = 0;
    virtual std::shared_ptr<IWorkspaceManager> getWorkspaceManager() = 0;
    virtual std::shared_ptr<IWallpaperSurface> createWallpaperSurface(std::shared_ptr<IMonitor> monitor) = 0;
};

} // namespace luma::platform
