#pragma once

namespace luma::render {

enum class NativeSurfaceType {
    Wayland,
    X11,
    Mock
};

struct NativeSurfaceInfo {
    NativeSurfaceType type{NativeSurfaceType::Mock};
    void* display{nullptr}; // wl_display* or xcb_connection_t*
    void* window{nullptr};  // wl_surface* or xcb_window_t*
};

class INativeSurfaceProvider {
public:
    virtual ~INativeSurfaceProvider() = default;

    virtual NativeSurfaceInfo getSurfaceInfo() const = 0;
    
    // Blocks the caller until the compositor/OS signals that it is ready for the next frame.
    // E.g., Wayland wl_surface_frame callback or X11 VSync event.
    virtual void waitForNextFrame() = 0;
};

} // namespace luma::render
