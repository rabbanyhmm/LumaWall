#pragma once
#include <memory>
#include <string>

namespace luma::render {

class IRenderDevice;

class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    virtual bool init() = 0;
    virtual void shutdown() = 0;
    
    virtual std::string getName() const = 0;
    
    // Creates the logical device which handles resources and contexts
    virtual std::unique_ptr<IRenderDevice> createDevice() = 0;
};

} // namespace luma::render
