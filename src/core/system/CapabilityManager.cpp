#include "CapabilityManager.hpp"
#include <core/SettingsManager.hpp>
#include <core/Logging.hpp>

namespace luma::core::system {

CapabilityManager& CapabilityManager::instance() {
    static CapabilityManager s_instance;
    return s_instance;
}

void CapabilityManager::detectCapabilities(const std::string& rendererName, const std::string& gpuName) {
    m_caps.renderer = rendererName;
    m_caps.gpuName = gpuName;

    // Hardcode some defaults for testing. In reality this might pull from Vulkan backend limits
    m_caps.fallbackRgbaAvailable = true;
    m_caps.fallbackSoftwareDecodeAvailable = true;
    m_caps.fallbackCpuUploadAvailable = true;

    spdlog::info("[SYSTEM] Hardware capabilities detected:\n{}", m_caps.toString());
}

bool CapabilityManager::shouldUseHardwareDecode() const {
    if (!SettingsManager::instance().useHardwareDecode()) return false;
    
    if (m_caps.supportsHardwareDecode) return true;
    
    spdlog::warn("[SYSTEM] Hardware decode requested but not supported. Falling back to Software.");
    return false;
}

bool CapabilityManager::shouldUseZeroCopy() const {
    if (!shouldUseHardwareDecode()) return false;
    
    if (m_caps.supportsZeroCopy) return true;

    spdlog::warn("[SYSTEM] Zero Copy requested but not supported. Falling back to CPU upload.");
    return false;
}

bool CapabilityManager::shouldUseYCbCr() const {
    if (m_caps.supportsYCbCr) return true;

    spdlog::warn("[SYSTEM] YCbCr requested but not supported. Falling back to RGBA conversion.");
    return false;
}

} // namespace luma::core::system
