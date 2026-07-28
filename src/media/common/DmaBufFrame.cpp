#include "DmaBufFrame.hpp"

namespace luma::media {

DmaBufFrame::DmaBufFrame(std::shared_ptr<AVFrame> avFrame, uint64_t formatModifier, std::vector<DmaBufPlane> planes)
    : m_avFrame(std::move(avFrame))
    , m_formatModifier(formatModifier)
    , m_planes(std::move(planes)) {
}

DmaBufFrame::~DmaBufFrame() {
    release();
}

void DmaBufFrame::release() {
    // When the shared_ptr goes out of scope, the custom deleter (av_frame_free) is called.
    m_avFrame.reset();
}

} // namespace luma::media
