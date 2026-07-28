#pragma once
#include <media/common/IHardwareFrame.hpp>
#include <vector>
#include <cstdint>
#include <memory>
#include <functional>

struct AVFrame;

namespace luma::media {

struct DmaBufPlane {
    int fd{-1};
    uint32_t offset{0};
    uint32_t pitch{0};
};

class DmaBufFrame : public IHardwareFrame {
public:
    DmaBufFrame(std::shared_ptr<AVFrame> avFrame, uint64_t formatModifier, std::vector<DmaBufPlane> planes);
    ~DmaBufFrame() override;

    HardwareFrameType getType() const override { return HardwareFrameType::DmaBuf; }
    void* getNativeHandle() const override { return const_cast<DmaBufFrame*>(this); }
    void release() override;

    uint64_t getFormatModifier() const { return m_formatModifier; }
    const std::vector<DmaBufPlane>& getPlanes() const { return m_planes; }

private:
    std::shared_ptr<AVFrame> m_avFrame;
    uint64_t m_formatModifier;
    std::vector<DmaBufPlane> m_planes;
};

} // namespace luma::media
