#pragma once
#include <cstdint>

namespace luma::render {

class ITexture;
class IRenderContext;

class IRenderSurface {
public:
    virtual ~IRenderSurface() = default;

    virtual bool build(uint32_t width, uint32_t height) = 0;
    virtual void destroy() = 0;

    // Acquire the next drawable texture/framebuffer
    virtual ITexture* acquireNextFrame(IRenderContext* context) = 0;

    // Present the currently acquired frame
    virtual void present(IRenderContext* context) = 0;

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
};

} // namespace luma::render
