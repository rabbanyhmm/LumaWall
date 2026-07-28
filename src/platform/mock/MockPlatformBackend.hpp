#pragma once

#include <platform/common/IPlatformBackend.hpp>

namespace luma::platform::mock {

class MockPlatformBackend : public IPlatformBackend {
public:
    BackendCapabilities getCapabilities() const noexcept override {
        return BackendCapabilities{
            .supportsLayerShell = true,
            .supportsWallpaperSurface = true,
            .supportsPerMonitor = true,
            .supportsWorkspaceEvents = true,
            .supportsHardwareZeroCopy = false
        };
    }
    
    bool init(std::shared_ptr<core::events::EventBus> eventBus) override {
        m_eventBus = eventBus;
        return true;
    }
    
    void pumpEvents() override {
        // Mock event pumping
    }

    std::shared_ptr<IDisplayManager> getDisplayManager() override { 
        return nullptr; // To be mocked
    }
    
    std::shared_ptr<IWorkspaceManager> getWorkspaceManager() override { 
        return nullptr; // To be mocked
    }

    std::shared_ptr<IWallpaperSurface> createWallpaperSurface(std::shared_ptr<IMonitor> monitor) override {
        (void)monitor;
        return nullptr;
    }

private:
    std::shared_ptr<core::events::EventBus> m_eventBus;
};

} // namespace luma::platform::mock
