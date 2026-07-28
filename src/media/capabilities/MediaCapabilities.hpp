#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace luma::media {

enum class HardwareDecoderType {
    None,
    VAAPI,
    NVDEC,
    QuickSync,
    VDPAU
};

struct MediaCapabilities {
    bool hasHardwareAcceleration{false};
    std::vector<HardwareDecoderType> availableHardwareDecoders;
    
    std::vector<std::string> supportedVideoCodecs;
    std::vector<std::string> supportedImageFormats;
    
    uint32_t maxDecodeWidth{0};
    uint32_t maxDecodeHeight{0};

    bool supportsVulkanVideo{false};
};

} // namespace luma::media
