#pragma once
#include <memory>
#include <media/common/Frame.hpp>
#include <render/common/ITexture.hpp>

namespace luma::render {

// Interface for importing frames into the renderer
class FrameImportManager {
public:
    virtual ~FrameImportManager() = default;

    // Attempts to import a frame natively. Returns false if unsupported or fallback is required.
    virtual bool importFrame(std::shared_ptr<media::Frame> frame) = 0;
};

} // namespace luma::render
