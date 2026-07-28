#include "GPUTimer.hpp"
#include <core/Logging.hpp>

namespace luma::render::vulkan {

GPUTimer::GPUTimer(std::shared_ptr<VulkanLogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice)) {
    
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_logicalDevice->getPhysicalDevice()->getHandle(), &props);
    m_timestampPeriod = props.limits.timestampPeriod;
}

void GPUTimer::reset(FrameContext* frameContext) {
    if (!frameContext || frameContext->getQueryPool() == VK_NULL_HANDLE) return;

    auto& data = m_frameData[frameContext];
    if (data.nextQueryIndex > 0) {
        vkCmdResetQueryPool(frameContext->getCommandBuffer(), frameContext->getQueryPool(), 0, data.nextQueryIndex);
    }
    data.ranges.clear();
    data.nextQueryIndex = 0;
}

void GPUTimer::startRecord(FrameContext* frameContext, const std::string& name) {
    if (!frameContext || frameContext->getQueryPool() == VK_NULL_HANDLE) return;
    
    auto& data = m_frameData[frameContext];
    if (data.nextQueryIndex >= 128) return; // Prevent overflow

    uint32_t startIndex = data.nextQueryIndex++;
    data.ranges[name].startIndex = startIndex;

    vkCmdWriteTimestamp(frameContext->getCommandBuffer(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frameContext->getQueryPool(), startIndex);
}

void GPUTimer::endRecord(FrameContext* frameContext, const std::string& name) {
    if (!frameContext || frameContext->getQueryPool() == VK_NULL_HANDLE) return;

    auto& data = m_frameData[frameContext];
    if (data.nextQueryIndex >= 128) return;

    uint32_t endIndex = data.nextQueryIndex++;
    data.ranges[name].endIndex = endIndex;

    vkCmdWriteTimestamp(frameContext->getCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frameContext->getQueryPool(), endIndex);
}

void GPUTimer::resolve(FrameContext* frameContext) {
    if (!frameContext || frameContext->getQueryPool() == VK_NULL_HANDLE) return;

    auto& data = m_frameData[frameContext];
    if (data.nextQueryIndex == 0) return;

    std::vector<uint64_t> timestamps(data.nextQueryIndex);
    VkResult res = vkGetQueryPoolResults(
        m_logicalDevice->getHandle(), 
        frameContext->getQueryPool(), 
        0, 
        data.nextQueryIndex, 
        timestamps.size() * sizeof(uint64_t), 
        timestamps.data(), 
        sizeof(uint64_t), 
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
    );

    if (res == VK_SUCCESS) {
        for (const auto& [name, range] : data.ranges) {
            uint64_t start = timestamps[range.startIndex];
            uint64_t end = timestamps[range.endIndex];
            if (end >= start) {
                float ms = static_cast<float>(end - start) * m_timestampPeriod * 1e-6f;
                m_timingsMS[name] = ms;
            }
        }
    } else {
        spdlog::warn("[VULKAN] Failed to get query pool results, error: {}", res);
    }
}

float GPUTimer::getTimeMS(const std::string& name) const {
    auto it = m_timingsMS.find(name);
    return it != m_timingsMS.end() ? it->second : 0.0f;
}

} // namespace luma::render::vulkan
