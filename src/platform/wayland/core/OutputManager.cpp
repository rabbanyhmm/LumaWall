#include "OutputManager.hpp"
#include <core/Logging.hpp>

namespace luma::platform::wayland::core {

static const wl_output_listener output_listener = {
    OutputManager::onGeometry,
    OutputManager::onMode,
    OutputManager::onDone,
    OutputManager::onScale,
    OutputManager::onName,
    OutputManager::onDescription
};

OutputManager::OutputManager(std::shared_ptr<luma::core::events::EventBus> eventBus) 
    : m_eventBus(std::move(eventBus)) {}

void OutputManager::addOutput(wl_registry* registry, uint32_t name, uint32_t version) {
    auto data = std::make_unique<OutputData>();
    data->globalName = name;
    data->manager = this;
    
    data->output.reset(static_cast<wl_output*>(
        wl_registry_bind(registry, name, &wl_output_interface, std::min(version, 4u))
    ));
    
    wl_output_add_listener(data->output.get(), &output_listener, data.get());
    m_outputs[name] = std::move(data);
}

void OutputManager::removeOutput(uint32_t name) {
    auto it = m_outputs.find(name);
    if (it != m_outputs.end()) {
        if (m_eventBus) {
            luma::core::events::MonitorRemovedEvent ev;
            ev.monitorId = it->second->nameId.empty() ? std::to_string(name) : it->second->nameId;
            m_eventBus->pushEvent(std::move(ev));
        }
        spdlog::info("[WAYLAND] Output removed: {}", name);
        m_outputs.erase(it);
    }
}

void OutputManager::onGeometry(void* data, wl_output* output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char* make, const char* model, int32_t transform) {
    auto* outData = static_cast<OutputData*>(data);
    outData->make = make ? make : "Unknown";
    outData->model = model ? model : "Unknown";
}

void OutputManager::onMode(void* data, wl_output* output, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
    if (flags & WL_OUTPUT_MODE_CURRENT) {
        auto* outData = static_cast<OutputData*>(data);
        outData->width = width;
        outData->height = height;
        outData->refreshRate = refresh;
    }
}

void OutputManager::onDone(void* data, wl_output* output) {
    auto* outData = static_cast<OutputData*>(data);
    spdlog::info("[WAYLAND] Output updated: {}x{}", outData->width, outData->height);
    
    if (outData->manager && outData->manager->m_eventBus) {
        luma::core::events::MonitorAddedEvent ev;
        ev.monitorId = outData->nameId.empty() ? std::to_string(outData->globalName) : outData->nameId;
        ev.name = outData->make + " " + outData->model;
        ev.width = outData->width;
        ev.height = outData->height;
        ev.refreshRate = outData->refreshRate / 1000;
        outData->manager->m_eventBus->pushEvent(std::move(ev));
    }
}

void OutputManager::onScale(void* data, wl_output* output, int32_t factor) {
    auto* outData = static_cast<OutputData*>(data);
    outData->scale = factor;
}

void OutputManager::onName(void* data, wl_output* output, const char* name) {
    auto* outData = static_cast<OutputData*>(data);
    outData->nameId = name ? name : "";
}

void OutputManager::onDescription(void* data, wl_output* output, const char* description) {
    // Optional
}

} // namespace luma::platform::wayland::core
