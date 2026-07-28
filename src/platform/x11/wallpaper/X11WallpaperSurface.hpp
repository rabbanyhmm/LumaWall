#pragma once
#include <platform/common/IWallpaperSurface.hpp>
#include <platform/x11/wallpaper/DesktopWindow.hpp>
#include <memory>

namespace luma::platform::x11::wallpaper {

class X11WallpaperSurface : public IWallpaperSurface {
public:
    X11WallpaperSurface(xcb_connection_t* conn, xcb_screen_t* screen, core::AtomCache* atomCache);
    ~X11WallpaperSurface() override = default;

    bool create(std::shared_ptr<IMonitor> monitor) override;
    void destroy() override;
    
    void resize(uint32_t width, uint32_t height) override;
    void present() override;
    
    void pause() override;
    void resume() override;
    
    void show() override;
    void hide() override;
    render::NativeSurfaceInfo getSurfaceInfo() const override {
        render::NativeSurfaceInfo info;
        info.type = render::NativeSurfaceType::X11;
        info.display = m_conn;
        info.window = reinterpret_cast<void*>(static_cast<uintptr_t>(m_window ? m_window->get() : 0));
        return info;
    }

    void waitForNextFrame() override {
        // X11 VSync synchronization is not natively event-driven without GLX/Present extensions.
        // We return immediately to rely on Vulkan FIFO blocking (vkAcquireNextImageKHR) for throttling.
    }

private:
    std::unique_ptr<DesktopWindow> m_window;
    xcb_connection_t* m_conn;
    xcb_screen_t* m_screen;
    core::AtomCache* m_atomCache;
};

} // namespace luma::platform::x11::wallpaper
