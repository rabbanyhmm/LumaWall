#pragma once

#include <string>
#include <cstdint>

namespace luma::platform {

class IMonitor {
public:
    virtual ~IMonitor() = default;

    virtual std::string getId() const = 0;
    virtual std::string getName() const = 0;
    
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual uint32_t getRefreshRate() const = 0;
    virtual float getScaleFactor() const = 0;
};

} // namespace luma::platform
