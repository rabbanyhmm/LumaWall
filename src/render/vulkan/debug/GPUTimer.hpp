#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <render/vulkan/frame/FrameContext.hpp>
#include <render/vulkan/device/VulkanLogicalDevice.hpp>

namespace luma::render::vulkan {

class GPUTimer {
public:
    GPUTimer(std::shared_ptr<VulkanLogicalDevice> logicalDevice);
    ~GPUTimer() = default;

    void reset(FrameContext* frameContext);
    
    // Call these to record times in the command buffer
    void startRecord(FrameContext* frameContext, const std::string& name);
    void endRecord(FrameContext* frameContext, const std::string& name);

    // Call this after waiting for the frame fence to read back the times
    void resolve(FrameContext* frameContext);

    float getTimeMS(const std::string& name) const;

private:
    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
    float m_timestampPeriod{1.0f};

    struct TimeRange {
        uint32_t startIndex;
        uint32_t endIndex;
    };

    struct FrameData {
        std::unordered_map<std::string, TimeRange> ranges;
        uint32_t nextQueryIndex{0};
    };

    // Keep track of data per frame
    std::unordered_map<FrameContext*, FrameData> m_frameData;
    
    // Store resolved timings
    std::unordered_map<std::string, float> m_timingsMS;
};

} // namespace luma::render::vulkan
