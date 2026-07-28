#pragma once
#include <string>
#include <cstdint>

namespace luma::media {

struct MediaCompatibilityReport {
    std::string codec{"Unknown"};
    uint32_t width{0};
    uint32_t height{0};
    uint32_t bitDepth{8};
    std::string pixelFormat{"Unknown"};
    
    bool hardwareDecodingActive{false};
    bool zeroCopyActive{false};
    
    std::string uploadPath{"Unknown"};
    std::string decoderBackend{"Unknown"};
    
    double averageDecodeLatencyMs{0.0};
    double averageRenderLatencyMs{0.0};
};

} // namespace luma::media
