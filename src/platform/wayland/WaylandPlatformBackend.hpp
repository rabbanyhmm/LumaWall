#pragma once

#include <platform/common/IPlatformBackend.hpp>
#include <platform/wayland/core/DisplayConnection.hpp>
#include <platform/wayland/core/Registry.hpp>
#include <platform/wayland/core/EventLoop.hpp>
#include <platform/wayland/WaylandWallpaperSurface.hpp>
#include <core/Logging.hpp>
#include <memory>

namespace luma::platform::wayland {

class WaylandPlatformBackend : public IPlatformBackend {
public:
    WaylandPlatformBackend() = default;
    
    BackendCapabilities getCapabilities() const noexcept override {
        return BackendCapabilities{
            .supportsLayerShell = true,
            .supportsWallpaperSurface = true,
            .supportsPerMonitor = true,
            .supportsWorkspaceEvents = true,
            .supportsHardwareZeroCopy = true
        };
    }
    
    bool init(std::shared_ptr<luma::core::events::EventBus> eventBus) override {
        m_eventBus = eventBus;
        
        if (!m_connection.connect().has_value()) {
            return false;
        }

        m_registry = std::make_unique<core::Registry>(m_connection.get(), m_eventBus);
        
        m_connection.roundtrip();

        if (!m_registry->getLayerShell()) {
            spdlog::warn("[WAYLAND] Compositor does not support wlr-layer-shell (e.g. GNOME Mutter). Falling back to X11/XWayland.");
            return false;
        }

        m_eventLoop = std::make_unique<core::EventLoop>(m_connection);
        m_eventLoop->start();

        spdlog::info("[WAYLAND] WaylandPlatformBackend fully initialized");
        return true;
    }
    
    void pumpEvents() override {
    }

    std::shared_ptr<IDisplayManager> getDisplayManager() override { 
        return nullptr;
    }
    
    std::shared_ptr<IWorkspaceManager> getWorkspaceManager() override { 
        return nullptr;
    }

    std::shared_ptr<IWallpaperSurface> createWallpaperSurface(std::shared_ptr<IMonitor> monitor) override {
        if (!m_registry) return nullptr;
        
        auto surface = std::make_shared<WaylandWallpaperSurface>(
            m_registry->getCompositor(),
            static_cast<protocols::LayerShell*>(m_registry->getLayerShell()),
            m_connection.get()
        );
        
        if (!surface->create(monitor)) {
            return nullptr;
        }
        return surface;
    }

private:
    std::shared_ptr<luma::core::events::EventBus> m_eventBus;
    core::DisplayConnection m_connection;
    std::unique_ptr<core::Registry> m_registry;
    std::unique_ptr<core::EventLoop> m_eventLoop;
};

} // namespace luma::platform::wayland
