#pragma once

#include <vector>
#include <memory>
#include "IMonitor.hpp"

namespace luma::platform {

class IDisplayManager {
public:
    virtual ~IDisplayManager() = default;

    // Get a list of all currently connected monitors
    virtual std::vector<std::shared_ptr<IMonitor>> getMonitors() const = 0;

    // Retrieve a specific monitor by ID
    virtual std::shared_ptr<IMonitor> getMonitor(const std::string& id) const = 0;
};

} // namespace luma::platform
