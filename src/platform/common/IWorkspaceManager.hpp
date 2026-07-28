#pragma once

#include <cstdint>

namespace luma::platform {

class IWorkspaceManager {
public:
    virtual ~IWorkspaceManager() = default;

    virtual uint32_t getActiveWorkspaceId() const = 0;
    virtual bool isWorkspaceVisible(uint32_t workspaceId) const = 0;
    
    // Returns true if there is a fullscreen application currently focused
    virtual bool hasFullscreenApplication() const = 0;
};

} // namespace luma::platform
