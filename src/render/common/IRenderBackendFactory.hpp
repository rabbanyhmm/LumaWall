#pragma once
#include <memory>
#include <string>

namespace luma::render {

class IRenderBackend;

class IRenderBackendFactory {
public:
    virtual ~IRenderBackendFactory() = default;

    virtual std::string getName() const = 0;
    virtual int getPriority() const = 0; // Higher means preferred
    virtual bool isSupported() const = 0;
    virtual std::unique_ptr<IRenderBackend> create() = 0;
};

} // namespace luma::render
