#include "MonitorManager.hpp"
#include <core/Logging.hpp>
#include <core/SettingsManager.hpp>
#include <platform/common/IMonitor.hpp>

namespace luma::app::monitor {

// Lightweight IMonitor built from a MonitorAddedEvent
class EventMonitor : public platform::IMonitor {
public:
    EventMonitor(const core::events::MonitorAddedEvent& ev)
        : m_id(ev.monitorId), m_name(ev.name),
          m_width(ev.width), m_height(ev.height), m_rate(ev.refreshRate) {}

    std::string getId() const override { return m_id; }
    std::string getName() const override { return m_name; }
    uint32_t getWidth() const override { return m_width ? m_width : 1920; }
    uint32_t getHeight() const override { return m_height ? m_height : 1080; }
    uint32_t getRefreshRate() const override { return m_rate ? m_rate : 60; }
    float getScaleFactor() const override { return 1.0f; }

private:
    std::string m_id, m_name;
    uint32_t m_width, m_height, m_rate;
};


MonitorManager::MonitorManager(
    platform::IPlatformBackend* platform,
    render::vulkan::VulkanDevice* vkDevice,
    std::shared_ptr<core::events::EventBus> eventBus)
    : m_platform(platform),
      m_vkDevice(vkDevice),
      m_eventBus(std::move(eventBus)) 
{
}

MonitorManager::~MonitorManager() {
    m_contexts.clear(); // Will invoke destructors which call stop() and cleanup
}

void MonitorManager::detectOutputs() {
    auto displayManager = m_platform->getDisplayManager();
    if (!displayManager) return;

    auto monitors = displayManager->getMonitors();
    
    // Check for removed monitors
    for (auto it = m_contexts.begin(); it != m_contexts.end(); ) {
        bool found = false;
        for (const auto& m : monitors) {
            if (m->getId() == it->first) {
                found = true;
                break;
            }
        }
        if (!found) {
            spdlog::info("[MONITOR] Monitor removed: {}", it->first);
            it = m_contexts.erase(it);
        } else {
            ++it;
        }
    }

    // Check for added monitors
    for (const auto& m : monitors) {
        if (m_contexts.find(m->getId()) == m_contexts.end()) {
            addMonitor(m);
        }
    }
}

void MonitorManager::addMonitor(std::shared_ptr<platform::IMonitor> monitor) {
    spdlog::info("[MONITOR] Monitor added: {} ({})", monitor->getName(), monitor->getId());
    auto context = std::make_shared<MonitorContext>(monitor, m_platform, m_vkDevice);
    if (context->init()) {
        m_contexts[monitor->getId()] = context;
        
        // Restore settings for this monitor
        std::string activeWall = core::SettingsManager::instance().getActiveWallpaper(monitor->getId());
        if (!activeWall.empty()) {
            context->loadWallpaper(activeWall);
        }
    } else {
        spdlog::error("[MONITOR] Failed to initialize context for monitor {}", monitor->getId());
    }
}

void MonitorManager::removeMonitor(const std::string& monitorId) {
    m_contexts.erase(monitorId);
}

void MonitorManager::loadSettingsAndRestore() {
    detectOutputs();
}




void MonitorManager::processEvents() {
    if (!m_eventBus) return;

    auto events = m_eventBus->pollAllEvents();
    for (const auto& event : events) {
        if (auto* ev = std::get_if<core::events::MonitorAddedEvent>(&event)) {
            if (m_contexts.find(ev->monitorId) == m_contexts.end()) {
                spdlog::info("[MONITOR] Hot-plug: monitor added {}", ev->monitorId);
                addMonitorFromEvent(*ev);
            }
        } else if (auto* ev = std::get_if<core::events::MonitorRemovedEvent>(&event)) {
            spdlog::info("[MONITOR] Hot-plug: monitor removed {}", ev->monitorId);
            removeMonitor(ev->monitorId);
        }
    }
}

void MonitorManager::addMonitorFromEvent(const core::events::MonitorAddedEvent& ev) {
    auto monitor = std::make_shared<EventMonitor>(ev);
    addMonitor(monitor);
}

void MonitorManager::playOnMonitor(const std::string& monitorId, const std::string& path) {
    if (monitorId.empty()) {
        // Play on all
        for (auto& [id, context] : m_contexts) {
            core::SettingsManager::instance().setActiveWallpaper(id, path);
            context->loadWallpaper(path);
        }
    } else {
        auto it = m_contexts.find(monitorId);
        if (it != m_contexts.end()) {
            core::SettingsManager::instance().setActiveWallpaper(monitorId, path);
            it->second->loadWallpaper(path);
        }
    }
}

void MonitorManager::pauseMonitor(const std::string& monitorId) {
    if (monitorId.empty()) {
        pauseAll();
    } else {
        auto it = m_contexts.find(monitorId);
        if (it != m_contexts.end()) {
            it->second->pause();
        }
    }
}

void MonitorManager::resumeMonitor(const std::string& monitorId) {
    if (monitorId.empty()) {
        resumeAll();
    } else {
        auto it = m_contexts.find(monitorId);
        if (it != m_contexts.end()) {
            it->second->resume();
        }
    }
}

void MonitorManager::pauseAll() {
    for (auto& [id, context] : m_contexts) {
        context->pause();
    }
}

void MonitorManager::resumeAll() {
    for (auto& [id, context] : m_contexts) {
        context->resume();
    }
}

} // namespace luma::app::monitor
