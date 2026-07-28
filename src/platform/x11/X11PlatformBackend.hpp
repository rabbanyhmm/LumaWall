#pragma once
#include <platform/common/IPlatformBackend.hpp>
#include <platform/x11/core/Connection.hpp>
#include <platform/x11/core/ScreenManager.hpp>
#include <platform/x11/core/AtomCache.hpp>
#include <platform/x11/core/EventLoop.hpp>
#include <platform/x11/randr/OutputManager.hpp>
#include <core/events/EventBus.hpp>
#include <memory>

namespace luma::platform::x11 {

class X11PlatformBackend : public luma::platform::IPlatformBackend {
public:
    X11PlatformBackend() = default;
    ~X11PlatformBackend() override;

    BackendCapabilities getCapabilities() const noexcept override;
    bool init(std::shared_ptr<luma::core::events::EventBus> eventBus) override;
    void pumpEvents() override;
    
    std::shared_ptr<IDisplayManager> getDisplayManager() override { return nullptr; }
    std::shared_ptr<IWorkspaceManager> getWorkspaceManager() override { return nullptr; }
    std::shared_ptr<luma::platform::IWallpaperSurface> createWallpaperSurface(std::shared_ptr<luma::platform::IMonitor> monitor) override;

private:
    std::shared_ptr<luma::core::events::EventBus> m_eventBus;
    core::Connection m_connection;
    std::unique_ptr<core::ScreenManager> m_screenManager;
    std::unique_ptr<core::AtomCache> m_atomCache;
    std::unique_ptr<randr::OutputManager> m_outputManager;
    std::unique_ptr<core::EventLoop> m_eventLoop;
};

} // namespace luma::platform::x11
