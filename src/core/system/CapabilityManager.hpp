#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace luma::core::system {

struct SystemCapabilities {
    std::string renderer;
    std::string gpuName;
    bool supportsHardwareDecode{false};
    bool supportsZeroCopy{false};
    bool supportsYCbCr{false};
    bool supportsTimelineSemaphores{false};
    bool supportsDynamicRendering{false};

    // Fallbacks
    bool fallbackRgbaAvailable{true};
    bool fallbackSoftwareDecodeAvailable{true};
    bool fallbackCpuUploadAvailable{true};

    std::string toString() const {
        std::stringstream ss;
        ss << "Renderer: " << renderer << "\n";
        ss << "GPU: " << gpuName << "\n";
        ss << "Hardware Decode: " << (supportsHardwareDecode ? "VA-API" : "None") << "\n";
        ss << "Zero Copy: " << (supportsZeroCopy ? "Yes" : "No") << "\n";
        ss << "YCbCr: " << (supportsYCbCr ? "Supported" : "Unsupported") << "\n";
        ss << "Timeline Semaphores: " << (supportsTimelineSemaphores ? "Yes" : "No") << "\n";
        ss << "Dynamic Rendering: " << (supportsDynamicRendering ? "Yes" : "No") << "\n\n";
        
        ss << "Fallbacks:\n";
        ss << (fallbackRgbaAvailable ? "✓" : "✗") << " RGBA conversion available\n";
        ss << (fallbackSoftwareDecodeAvailable ? "✓" : "✗") << " Software decoding available\n";
        ss << (fallbackCpuUploadAvailable ? "✓" : "✗") << " CPU upload available\n";
        return ss.str();
    }
};

class CapabilityManager {
public:
    static CapabilityManager& instance();

    void detectCapabilities(const std::string& rendererName, const std::string& gpuName);
    
    // Setters for detection phases
    void setHardwareDecode(bool supported) { m_caps.supportsHardwareDecode = supported; }
    void setZeroCopy(bool supported) { m_caps.supportsZeroCopy = supported; }
    void setYCbCr(bool supported) { m_caps.supportsYCbCr = supported; }
    void setTimelineSemaphores(bool supported) { m_caps.supportsTimelineSemaphores = supported; }
    void setDynamicRendering(bool supported) { m_caps.supportsDynamicRendering = supported; }

    const SystemCapabilities& getCapabilities() const { return m_caps; }

    // Logic for deciding paths
    bool shouldUseHardwareDecode() const;
    bool shouldUseZeroCopy() const;
    bool shouldUseYCbCr() const;

private:
    CapabilityManager() = default;
    ~CapabilityManager() = default;

    SystemCapabilities m_caps;
};

} // namespace luma::core::system
