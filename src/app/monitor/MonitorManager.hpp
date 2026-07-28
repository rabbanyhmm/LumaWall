#pragma once

#include <memory>
#include <string>
#include <map>
#include "MonitorContext.hpp"
#include <platform/common/IDisplayManager.hpp>
#include <platform/common/IPlatformBackend.hpp>
#include <render/vulkan/VulkanDevice.hpp>
#include <core/events/EventBus.hpp>
#include <core/events/Events.hpp>

namespace luma::app::monitor {

class MonitorManager {
public:
    MonitorManager(
        platform::IPlatformBackend* platform,
        render::vulkan::VulkanDevice* vkDevice,
        std::shared_ptr<core::events::EventBus> eventBus
    );
    ~MonitorManager();

    void detectOutputs();
    void loadSettingsAndRestore();
    void processEvents();  // Call each frame from main loop to handle hotplug

    void playOnMonitor(const std::string& monitorId, const std::string& path);
    void pauseMonitor(const std::string& monitorId);
    void resumeMonitor(const std::string& monitorId);

    void pauseAll();
    void resumeAll();

private:
    void addMonitor(std::shared_ptr<platform::IMonitor> monitor);
    void addMonitorFromEvent(const core::events::MonitorAddedEvent& ev);
    void removeMonitor(const std::string& monitorId);

    platform::IPlatformBackend* m_platform;
    render::vulkan::VulkanDevice* m_vkDevice;
    std::shared_ptr<core::events::EventBus> m_eventBus;

    std::map<std::string, std::shared_ptr<MonitorContext>> m_contexts;
};

} // namespace luma::app::monitor
