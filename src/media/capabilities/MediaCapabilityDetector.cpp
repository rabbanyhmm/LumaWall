#include "MediaCapabilityDetector.hpp"
#include <core/Logging.hpp>
#include <filesystem>

namespace luma::media {

MediaCapabilities MediaCapabilityDetector::detect() {
    MediaCapabilities caps;
    
    // In a real scenario, this would query FFmpeg (avcodec_configuration), 
    // libva (vaQueryDisplayAttributes), or Vulkan Video extensions.
    
    // Mock static capabilities for now
    caps.hasHardwareAcceleration = false; 
    
    // Try to detect VAAPI trivially by checking if /dev/dri/renderD128 exists
    if (std::filesystem::exists("/dev/dri/renderD128")) {
        caps.hasHardwareAcceleration = true;
        caps.availableHardwareDecoders.push_back(HardwareDecoderType::VAAPI);
        spdlog::info("[MEDIA] Detected potential VAAPI hardware acceleration.");
    }

    caps.supportedVideoCodecs = {"h264", "hevc", "vp9", "av1"};
    caps.supportedImageFormats = {"png", "jpg", "jpeg", "gif", "webp", "apng"};
    
    // Reasonable defaults for modern systems
    caps.maxDecodeWidth = 4096;
    caps.maxDecodeHeight = 4096;
    
    caps.supportsVulkanVideo = false; // We can detect VK_KHR_video_decode_queue later

    spdlog::info("[MEDIA] MediaCapabilityDetector initialized.");
    spdlog::info("[MEDIA] Hardware Acceleration: {}", caps.hasHardwareAcceleration);
    spdlog::info("[MEDIA] Max Decode Resolution: {}x{}", caps.maxDecodeWidth, caps.maxDecodeHeight);
    
    return caps;
}

} // namespace luma::media
