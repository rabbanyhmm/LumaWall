#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <media/common/IHardwareFrame.hpp>

namespace luma::media {

enum class PixelFormat {
    RGBA8,
    BGRA8,
    RGB8,
    NV12,
    YUV420P,
    UNKNOWN
};

enum class ColorSpace {
    BT709,
    BT2020,
    SRGB,
    UNKNOWN
};

struct Frame {
    uint32_t width{0};
    uint32_t height{0};
    PixelFormat format{PixelFormat::UNKNOWN};
    ColorSpace colorSpace{ColorSpace::UNKNOWN};
    
    double timestamp{0.0}; // Presentation timestamp in seconds
    double duration{0.0};  // Frame duration in seconds

    std::vector<uint8_t> data;
    
    // For planar formats like NV12/YUV
    std::vector<uint8_t> dataPlane1; 
    std::vector<uint8_t> dataPlane2;

    // Hardware frame (e.g. DMABUF). If not null, the CPU vectors are typically empty.
    std::shared_ptr<IHardwareFrame> hwFrame{nullptr};

    bool isValid() const { 
        return width > 0 && height > 0 && (!data.empty() || hwFrame != nullptr); 
    }
    
    bool isHardwareBacked() const { return hwFrame != nullptr; }
};

} // namespace luma::media
