#pragma once
#include <cstdint>

namespace luma::render {

enum class TextureFormat {
    RGBA8,
    BGRA8,
    NV12,
    YUV420,
    RGBA16F // HDR
};

class ITexture {
public:
    virtual ~ITexture() = default;

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual TextureFormat getFormat() const = 0;
};

} // namespace luma::render
