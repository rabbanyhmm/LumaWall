#include "LayerShell.hpp"
#define namespace namespace_
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#undef namespace

namespace luma::platform::wayland::protocols {

void LayerShell::LayerSurfaceDeleter::operator()(zwlr_layer_surface_v1* ptr) const noexcept {
    if (ptr) {
        zwlr_layer_surface_v1_destroy(ptr);
    }
}

LayerShell::LayerShell(zwlr_layer_shell_v1* layerShell) : m_layerShell(layerShell) {}

LayerShell::ScopedLayerSurface LayerShell::createBackgroundSurface(wl_surface* surface, wl_output* output, const char* namespace_name) {
    if (!m_layerShell) return nullptr;

    zwlr_layer_surface_v1* layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        m_layerShell, 
        surface, 
        output, 
        ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND, 
        namespace_name
    );

    if (layer_surface) {
        zwlr_layer_surface_v1_set_anchor(layer_surface, 
            ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
            ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
            ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
            ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
            
        zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, -1);
        zwlr_layer_surface_v1_set_keyboard_interactivity(layer_surface, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);
    }

    return ScopedLayerSurface(layer_surface);
}

} // namespace luma::platform::wayland::protocols
