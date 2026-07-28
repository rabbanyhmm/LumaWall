#include "FrameContext.hpp"
#include <core/Logging.hpp>
#include <render/vulkan/debug/VulkanDebugUtils.hpp>
#include <string>

namespace luma::render::vulkan {

FrameContext::FrameContext(std::shared_ptr<VulkanLogicalDevice> logicalDevice, std::shared_ptr<CommandPool> commandPool, uint32_t frameIndex)
    : m_logicalDevice(std::move(logicalDevice)), m_commandPool(std::move(commandPool)), m_frameIndex(frameIndex) {
}

FrameContext::~FrameContext() {
    cleanup();
}

bool FrameContext::init() {
    auto device = m_logicalDevice->getHandle();

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &m_inFlightFence) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create synchronization objects for frame {}", m_frameIndex);
        return false;
    }

    m_commandBuffer = m_commandPool->allocateBuffer();
    if (m_commandBuffer == VK_NULL_HANDLE) {
        return false;
    }

    // Create Query Pool
    VkQueryPoolCreateInfo queryPoolInfo{};
    queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    queryPoolInfo.queryCount = 128;

    if (vkCreateQueryPool(m_logicalDevice->getHandle(), &queryPoolInfo, nullptr, &m_queryPool) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create query pool for frame {}", m_frameIndex);
        return false;
    }

    // Set debug names
    std::string prefix = "Frame " + std::to_string(m_frameIndex) + " ";
    VulkanDebugUtils::setObjectName(device, m_imageAvailableSemaphore, VK_OBJECT_TYPE_SEMAPHORE, prefix + "ImageAvailable");
    VulkanDebugUtils::setObjectName(device, m_renderFinishedSemaphore, VK_OBJECT_TYPE_SEMAPHORE, prefix + "RenderFinished");
    VulkanDebugUtils::setObjectName(device, m_inFlightFence, VK_OBJECT_TYPE_FENCE, prefix + "InFlight");
    VulkanDebugUtils::setObjectName(device, m_commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, prefix + "CmdBuf");

    return true;
}

void FrameContext::cleanup() {
    auto device = m_logicalDevice->getHandle();

    if (m_renderFinishedSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, m_renderFinishedSemaphore, nullptr);
        m_renderFinishedSemaphore = VK_NULL_HANDLE;
    }
    if (m_imageAvailableSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, m_imageAvailableSemaphore, nullptr);
        m_imageAvailableSemaphore = VK_NULL_HANDLE;
    }
    if (m_inFlightFence != VK_NULL_HANDLE) {
        vkDestroyFence(device, m_inFlightFence, nullptr);
        m_inFlightFence = VK_NULL_HANDLE;
    }

    if (m_commandBuffer != VK_NULL_HANDLE) {
        m_commandPool->freeBuffer(m_commandBuffer);
        m_commandBuffer = VK_NULL_HANDLE;
    }
    if (m_queryPool != VK_NULL_HANDLE) {
        vkDestroyQueryPool(m_logicalDevice->getHandle(), m_queryPool, nullptr);
        m_queryPool = VK_NULL_HANDLE;
    }
    m_deletionQueue.flush();
}

void FrameContext::waitForFence() {
    vkWaitForFences(m_logicalDevice->getHandle(), 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
}

void FrameContext::resetFence() {
    vkResetFences(m_logicalDevice->getHandle(), 1, &m_inFlightFence);
    m_deletionQueue.flush();
}

} // namespace luma::render::vulkan
