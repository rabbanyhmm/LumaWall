#pragma once
#include <memory>
#include <cstdint>
#include <render/common/INativeSurfaceProvider.hpp>

namespace luma::render {

class IRenderContext;
class IRenderSurface;
class ITexture;
class IBuffer;
class IShader;

class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

    virtual void waitIdle() = 0;

    virtual std::unique_ptr<IRenderSurface> createSurface(INativeSurfaceProvider* surfaceProvider) = 0;

    // Creates a context for recording commands
    virtual std::unique_ptr<IRenderContext> createContext() = 0;
};

} // namespace luma::render
