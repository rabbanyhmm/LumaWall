#include "CommandPool.hpp"
#include <core/Logging.hpp>

namespace luma::render::vulkan {

CommandPool::CommandPool(std::shared_ptr<VulkanLogicalDevice> logicalDevice, uint32_t queueFamilyIndex)
    : m_logicalDevice(std::move(logicalDevice)), m_queueFamilyIndex(queueFamilyIndex) {
}

CommandPool::~CommandPool() {
    cleanup();
}

bool CommandPool::init() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_queueFamilyIndex;

    if (vkCreateCommandPool(m_logicalDevice->getHandle(), &poolInfo, nullptr, &m_pool) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create command pool");
        return false;
    }
    return true;
}

void CommandPool::cleanup() {
    if (m_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_logicalDevice->getHandle(), m_pool, nullptr);
        m_pool = VK_NULL_HANDLE;
    }
}

VkCommandBuffer CommandPool::allocateBuffer() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(m_logicalDevice->getHandle(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to allocate command buffer");
        return VK_NULL_HANDLE;
    }
    return commandBuffer;
}

void CommandPool::freeBuffer(VkCommandBuffer buffer) {
    if (buffer != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(m_logicalDevice->getHandle(), m_pool, 1, &buffer);
    }
}

} // namespace luma::render::vulkan
