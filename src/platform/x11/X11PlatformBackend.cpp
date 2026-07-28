#include "X11PlatformBackend.hpp"
#include <platform/x11/wallpaper/X11WallpaperSurface.hpp>
#include <core/Logging.hpp>

namespace luma::platform::x11 {

X11PlatformBackend::~X11PlatformBackend() {
    if (m_eventLoop) m_eventLoop->stop();
    m_connection.disconnect();
}

BackendCapabilities X11PlatformBackend::getCapabilities() const noexcept {
    return BackendCapabilities{
        .supportsLayerShell = false,
        .supportsWallpaperSurface = true,
        .supportsPerMonitor = true,
        .supportsWorkspaceEvents = true,
        .supportsHardwareZeroCopy = false
    };
}

bool X11PlatformBackend::init(std::shared_ptr<luma::core::events::EventBus> eventBus) {
    m_eventBus = std::move(eventBus);

    if (!m_connection.connect().has_value()) {
        return false;
    }

    m_screenManager = std::make_unique<core::ScreenManager>(m_connection.get(), m_connection.getDefaultScreen());
    
    m_atomCache = std::make_unique<core::AtomCache>(m_connection.get());
    m_atomCache->prefetch({
        "_NET_WM_WINDOW_TYPE",
        "_NET_WM_WINDOW_TYPE_DESKTOP",
        "_NET_WM_STATE",
        "_NET_WM_STATE_BELOW"
    });

    m_outputManager = std::make_unique<randr::OutputManager>(m_connection.get(), m_screenManager->getDefaultScreen(), m_eventBus);
    m_outputManager->init();

    m_eventLoop = std::make_unique<core::EventLoop>(m_connection.get());
    m_eventLoop->start([this](xcb_generic_event_t* event) {
        m_outputManager->handleRandrEvent(event);
    });

    spdlog::info("[X11] X11PlatformBackend initialized");
    return true;
}

void X11PlatformBackend::pumpEvents() {
    // Handled by the EventLoop thread asynchronously
}

std::shared_ptr<luma::platform::IWallpaperSurface> X11PlatformBackend::createWallpaperSurface(std::shared_ptr<luma::platform::IMonitor> monitor) {
    auto surface = std::make_shared<wallpaper::X11WallpaperSurface>(
        m_connection.get(), 
        m_screenManager->getDefaultScreen(), 
        m_atomCache.get()
    );
    
    if (!surface->create(monitor)) {
        return nullptr;
    }
    return surface;
}

} // namespace luma::platform::x11
