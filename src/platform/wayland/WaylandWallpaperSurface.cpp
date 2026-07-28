#include "WaylandWallpaperSurface.hpp"
#include <core/Logging.hpp>
#include <wayland-client.h>
#include <thread>
#include <chrono>

namespace luma::platform::wayland {

WaylandWallpaperSurface::WaylandWallpaperSurface(wl_compositor* compositor, protocols::LayerShell* layerShell, wl_display* display)
    : m_compositor(compositor), m_layerShell(layerShell), m_display(display) {}

WaylandWallpaperSurface::~WaylandWallpaperSurface() {
    destroy();
}

bool WaylandWallpaperSurface::create(std::shared_ptr<IMonitor> monitor) {
    if (!m_compositor || !m_layerShell) {
        return false;
    }

    m_surface.reset(wl_compositor_create_surface(m_compositor));
    if (!m_surface) {
        return false;
    }

    // Provide nullptr output to let the compositor decide, or bind to specific output later
    wl_output* targetOutput = nullptr; 

    m_layerSurface = m_layerShell->createBackgroundSurface(m_surface.get(), targetOutput);
    if (!m_layerSurface) {
        m_surface.reset();
        return false;
    }

    wl_surface_commit(m_surface.get());
    spdlog::info("[WAYLAND] WaylandWallpaperSurface created");
    
    return true;
}

void WaylandWallpaperSurface::destroy() {
    m_layerSurface.reset();
    m_surface.reset();
}

void WaylandWallpaperSurface::resize(uint32_t width, uint32_t height) {}
void WaylandWallpaperSurface::present() {}
void WaylandWallpaperSurface::hide() {}
void WaylandWallpaperSurface::show() {}
void WaylandWallpaperSurface::pause() {}
void WaylandWallpaperSurface::resume() {}

render::NativeSurfaceInfo WaylandWallpaperSurface::getSurfaceInfo() const {
    render::NativeSurfaceInfo info;
    info.type = render::NativeSurfaceType::Wayland;
    
    // We would need the display from DisplayConnection. 
    // For now we'll just return surface since wayland egl often needs both, 
    // but VK_KHR_wayland_surface takes wl_display and wl_surface.
    // We'll wire wl_display up properly later, stubbing it out for a moment.
    info.display = m_display; 
    info.window = m_surface.get();
    return info;
}

void WaylandWallpaperSurface::waitForNextFrame() {
    // We need to use wl_surface_frame callback here to wait for next frame.
    // For now, to keep the milestone isolated without introducing Wayland threading lockups,
    // we return immediately, relying on Vulkan FIFO blocking (vkAcquireNextImageKHR).
    // The strict implementation requires the wayland event loop dispatching.
}

} // namespace luma::platform::wayland
