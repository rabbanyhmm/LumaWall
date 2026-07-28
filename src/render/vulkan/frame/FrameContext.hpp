#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <render/vulkan/device/VulkanLogicalDevice.hpp>
#include <render/vulkan/command/CommandPool.hpp>
#include <render/common/DeletionQueue.hpp>

namespace luma::render::vulkan {

class FrameContext {
public:
    FrameContext(std::shared_ptr<VulkanLogicalDevice> logicalDevice, std::shared_ptr<CommandPool> commandPool, uint32_t frameIndex);
    ~FrameContext();

    bool init();
    void cleanup();

    void waitForFence();
    void resetFence();

    VkCommandBuffer getCommandBuffer() const { return m_commandBuffer; }
    VkSemaphore getImageAvailableSemaphore() const { return m_imageAvailableSemaphore; }
    VkSemaphore getRenderFinishedSemaphore() const { return m_renderFinishedSemaphore; }
    VkFence getInFlightFence() const { return m_inFlightFence; }
    VkQueryPool getQueryPool() const { return m_queryPool; }
    uint32_t getFrameIndex() const { return m_frameIndex; }
    DeletionQueue& getDeletionQueue() { return m_deletionQueue; }

private:
    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
    std::shared_ptr<CommandPool> m_commandPool;
    uint32_t m_frameIndex;

    VkCommandBuffer m_commandBuffer{VK_NULL_HANDLE};
    VkSemaphore m_imageAvailableSemaphore{VK_NULL_HANDLE};
    VkSemaphore m_renderFinishedSemaphore{VK_NULL_HANDLE};
    VkFence m_inFlightFence{VK_NULL_HANDLE};
    VkQueryPool m_queryPool{VK_NULL_HANDLE};
    DeletionQueue m_deletionQueue;
};

} // namespace luma::render::vulkan
