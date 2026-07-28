#include "PlatformFactory.hpp"
#include <platform/wayland/WaylandPlatformBackend.hpp>
#include <platform/x11/X11PlatformBackend.hpp>
#include <core/Logging.hpp>
#include <cstdlib>

namespace luma::platform {

std::unique_ptr<IPlatformBackend> PlatformFactory::create() {
    const char* waylandDisplay = std::getenv("WAYLAND_DISPLAY");
    if (waylandDisplay && waylandDisplay[0] != '\0') {
        spdlog::info("[PLATFORM] Detected Wayland environment");
        return std::make_unique<wayland::WaylandPlatformBackend>();
    } else {
        spdlog::info("[PLATFORM] Detected X11 environment");
        return std::make_unique<x11::X11PlatformBackend>();
    }
}

} // namespace luma::platform
