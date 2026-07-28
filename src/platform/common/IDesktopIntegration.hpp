#pragma once

namespace luma::platform::common {

enum class DesktopState {
    Normal,
    Overview,
    Locked
};

class IDesktopIntegration {
public:
    virtual ~IDesktopIntegration() = default;

    virtual bool initialize() = 0;
    virtual DesktopState getState() = 0;
    
    // Optional callbacks that the main application can hook into
    // Alternatively, this subsystem can broadcast these via EventBus
    virtual void onOverviewChanged(bool active) = 0;
};

} // namespace luma::platform::common
