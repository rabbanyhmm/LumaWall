#pragma once
#include <cstdint>

namespace luma::render {

class IRenderContext;
class IRenderSurface;
class ITexture;

struct RenderFrame {
    IRenderContext* context{nullptr};
    IRenderSurface* surface{nullptr};
    ITexture* targetTexture{nullptr};
    uint64_t frameIndex{0};
    double deltaTime{0.0};
};

} // namespace luma::render
