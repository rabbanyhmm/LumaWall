#pragma once

#include <cstdint>
#include <memory>
#include "IMonitor.hpp"

#include <render/common/INativeSurfaceProvider.hpp>

namespace luma::platform {

class IWallpaperSurface : public render::INativeSurfaceProvider {
public:
    virtual ~IWallpaperSurface() = default;

    virtual bool create(std::shared_ptr<IMonitor> monitor) = 0;
    virtual void destroy() = 0;

    virtual void resize(uint32_t width, uint32_t height) = 0;
    
    // Notifies compositor of a new frame if necessary
    virtual void present() = 0;

    virtual void hide() = 0;
    virtual void show() = 0;

    virtual void pause() = 0;
    virtual void resume() = 0;

    // from INativeSurfaceProvider
    // virtual render::NativeSurfaceInfo getSurfaceInfo() const = 0;
    // virtual void waitForNextFrame() = 0;
};

} // namespace luma::platform
