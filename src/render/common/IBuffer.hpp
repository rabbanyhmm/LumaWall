#pragma once
#include <cstdint>

namespace luma::render {

enum class BufferUsage {
    Vertex,
    Index,
    Uniform,
    TransferSrc,
    TransferDst
};

class IBuffer {
public:
    virtual ~IBuffer() = default;

    virtual size_t getSize() const = 0;
    virtual BufferUsage getUsage() const = 0;
    
    // Map memory for CPU writes (if supported)
    virtual void* map() = 0;
    virtual void unmap() = 0;
};

} // namespace luma::render
