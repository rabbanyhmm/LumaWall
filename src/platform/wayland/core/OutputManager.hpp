#pragma once
#include <platform/wayland/wrapper/ScopedProxy.hpp>
#include <core/events/EventBus.hpp>
#include <map>
#include <memory>
#include <string>

namespace luma::platform::wayland::core {

class OutputManager {
public:
    OutputManager(std::shared_ptr<luma::core::events::EventBus> eventBus);
    ~OutputManager() = default;

    void addOutput(wl_registry* registry, uint32_t name, uint32_t version);
    void removeOutput(uint32_t name);

private:
    struct OutputData {
        wrapper::ScopedOutput output;
        std::string make;
        std::string model;
        std::string nameId; // from onName (version 4)
        int32_t width{0};
        int32_t height{0};
        int32_t refreshRate{0};
        int32_t scale{1};
        uint32_t globalName{0};
        OutputManager* manager{nullptr};
    };

public:
    static void onGeometry(void* data, wl_output* output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char* make, const char* model, int32_t transform);
    static void onMode(void* data, wl_output* output, uint32_t flags, int32_t width, int32_t height, int32_t refresh);
    static void onDone(void* data, wl_output* output);
    static void onScale(void* data, wl_output* output, int32_t factor);
    static void onName(void* data, wl_output* output, const char* name);
    static void onDescription(void* data, wl_output* output, const char* description);

private:
    std::shared_ptr<luma::core::events::EventBus> m_eventBus;
    std::map<uint32_t, std::unique_ptr<OutputData>> m_outputs;
};

} // namespace luma::platform::wayland::core
