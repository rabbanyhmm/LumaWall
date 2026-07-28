#pragma once

#include <string>
#include <cstdint>
#include <variant>

namespace luma::core::events {

struct MonitorAddedEvent {
    std::string monitorId;
    std::string name;
    uint32_t width{0};
    uint32_t height{0};
    uint32_t refreshRate{60};
};

struct MonitorRemovedEvent {
    std::string monitorId;
};

struct ResolutionChangedEvent {
    std::string monitorId;
    uint32_t newWidth{0};
    uint32_t newHeight{0};
};

struct WorkspaceChangedEvent {
    uint32_t workspaceId{0};
    bool isVisible{true};
};

struct SuspendEvent {};
struct ResumeEvent {};
struct LockEvent {};
struct UnlockEvent {};

struct FullscreenApplicationEvent {
    std::string monitorId;
    bool isFullscreen{false};
};

struct BatteryChangedEvent {
    bool isLow{false};
    bool isCharging{false};
};

using PlatformEvent = std::variant<
    MonitorAddedEvent,
    MonitorRemovedEvent,
    ResolutionChangedEvent,
    WorkspaceChangedEvent,
    SuspendEvent,
    ResumeEvent,
    LockEvent,
    UnlockEvent,
    FullscreenApplicationEvent,
    BatteryChangedEvent
>;

} // namespace luma::core::events
