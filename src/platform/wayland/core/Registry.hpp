#pragma once
#include <platform/wayland/wrapper/ScopedProxy.hpp>
#include <vector>
#include <string>
#include <memory>
#include <core/events/EventBus.hpp>
#include "OutputManager.hpp"

namespace luma::platform::wayland::core {

class Registry {
public:
    Registry(wl_display* display, std::shared_ptr<luma::core::events::EventBus> eventBus = nullptr);
    ~Registry();

    void roundtrip();

    wl_compositor* getCompositor() const { return m_compositor.get(); }
    wl_shm* getShm() const { return m_shm.get(); }
    void* getLayerShell() const { return m_layerShell; }

public:
    static void onGlobal(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
    static void onGlobalRemove(void* data, wl_registry* registry, uint32_t name);

private:
    wrapper::ScopedRegistry m_registry;
    wl_display* m_display{nullptr};

    std::unique_ptr<OutputManager> m_outputManager;

    wrapper::ScopedCompositor m_compositor;
    wrapper::ScopedShm m_shm;
    
    // Stored as void* in header to avoid requiring protocol headers globally
    void* m_layerShell{nullptr}; 
};

} // namespace luma::platform::wayland::core
