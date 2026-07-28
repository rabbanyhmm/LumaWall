#pragma once
#include <platform/common/IWallpaperSurface.hpp>
#include "protocols/LayerShell.hpp"

struct wl_compositor;

namespace luma::platform::wayland {

class WaylandWallpaperSurface : public IWallpaperSurface {
public:
    WaylandWallpaperSurface(wl_compositor* compositor, protocols::LayerShell* layerShell, wl_display* display);
    ~WaylandWallpaperSurface() override;

    bool create(std::shared_ptr<IMonitor> monitor) override;
    void destroy() override;
    void resize(uint32_t width, uint32_t height) override;
    void present() override;
    void hide() override;
    void show() override;
    void pause() override;
    void resume() override;

    render::NativeSurfaceInfo getSurfaceInfo() const override;
    void waitForNextFrame() override;

private:
    wl_compositor* m_compositor;
    wl_display* m_display{nullptr};
    protocols::LayerShell* m_layerShell;
    
    wrapper::ScopedSurface m_surface;
    protocols::LayerShell::ScopedLayerSurface m_layerSurface;
};

} // namespace luma::platform::wayland
