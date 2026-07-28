#pragma once
#include <platform/wayland/wrapper/ScopedProxy.hpp>

struct zwlr_layer_shell_v1;
struct zwlr_layer_surface_v1;
struct wl_surface;
struct wl_output;

namespace luma::platform::wayland::protocols {

class LayerShell {
public:
    LayerShell(zwlr_layer_shell_v1* layerShell);
    ~LayerShell() = default;

    struct LayerSurfaceDeleter {
        void operator()(zwlr_layer_surface_v1* ptr) const noexcept;
    };
    using ScopedLayerSurface = std::unique_ptr<zwlr_layer_surface_v1, LayerSurfaceDeleter>;

    ScopedLayerSurface createBackgroundSurface(wl_surface* surface, wl_output* output, const char* namespace_name = "lumawall");

private:
    zwlr_layer_shell_v1* m_layerShell{nullptr};
};

} // namespace luma::platform::wayland::protocols
