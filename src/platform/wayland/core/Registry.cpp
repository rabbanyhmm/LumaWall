#include "Registry.hpp"
#include <core/Logging.hpp>
#include <cstring>

#define namespace namespace_
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#undef namespace

namespace luma::platform::wayland::core {

static const wl_registry_listener registry_listener = {
    Registry::onGlobal,
    Registry::onGlobalRemove
};

Registry::Registry(wl_display* display, std::shared_ptr<luma::core::events::EventBus> eventBus) 
    : m_display(display), m_outputManager(std::make_unique<OutputManager>(std::move(eventBus))) {
    m_registry.reset(wl_display_get_registry(m_display));
    wl_registry_add_listener(m_registry.get(), &registry_listener, this);
}

Registry::~Registry() {
    if (m_layerShell) {
        zwlr_layer_shell_v1_destroy(static_cast<zwlr_layer_shell_v1*>(m_layerShell));
    }
}

void Registry::roundtrip() {
    wl_display_roundtrip(m_display);
}

void Registry::onGlobal(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    auto* self = static_cast<Registry*>(data);

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        self->m_compositor.reset(static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 4u))
        ));
        spdlog::info("[WAYLAND] Bound global wl_compositor");
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        self->m_shm.reset(static_cast<wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1)
        ));
        spdlog::info("[WAYLAND] Bound global wl_shm");
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        self->m_layerShell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
        spdlog::info("[WAYLAND] Bound global zwlr_layer_shell_v1");
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        if (self->m_outputManager) {
            self->m_outputManager->addOutput(registry, name, version);
        }
    }
}

void Registry::onGlobalRemove(void* data, wl_registry* registry, uint32_t name) {
    auto* self = static_cast<Registry*>(data);
    if (self->m_outputManager) {
        self->m_outputManager->removeOutput(name);
    }
}

} // namespace luma::platform::wayland::core
