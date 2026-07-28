#pragma once
#include <memory>
#include <media/common/Frame.hpp>

namespace luma::media {

class IFrameProvider {
public:
    virtual ~IFrameProvider() = default;

    // The renderer asks: "Which frame should be displayed now?"
    virtual std::shared_ptr<Frame> getNextFrame() = 0;
};

} // namespace luma::media
