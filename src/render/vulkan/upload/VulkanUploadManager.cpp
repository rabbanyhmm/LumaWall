#include "VulkanUploadManager.hpp"
#include <core/Logging.hpp>
#include <render/vulkan/debug/VulkanDebugUtils.hpp>

namespace luma::render::vulkan {

VulkanUploadManager::VulkanUploadManager(std::shared_ptr<VulkanLogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice)) {}

VulkanUploadManager::~VulkanUploadManager() {
    cleanup();
}

bool VulkanUploadManager::init() {
    auto device = m_logicalDevice->getHandle();
    auto family = m_logicalDevice->getPhysicalDevice()->getQueueFamilies().graphicsFamily.value();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = family;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_uploadContext.commandPool) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create upload command pool");
        return false;
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_uploadContext.commandPool;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &m_uploadContext.commandBuffer) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to allocate upload command buffer");
        return false;
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    
    if (vkCreateFence(device, &fenceInfo, nullptr, &m_uploadContext.uploadFence) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create upload fence");
        return false;
    }

    VulkanDebugUtils::setObjectName(device, m_uploadContext.commandPool, VK_OBJECT_TYPE_COMMAND_POOL, "UploadCommandPool");
    VulkanDebugUtils::setObjectName(device, m_uploadContext.commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "UploadCommandBuffer");
    VulkanDebugUtils::setObjectName(device, m_uploadContext.uploadFence, VK_OBJECT_TYPE_FENCE, "UploadFence");

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_ringSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // Maps host visible memory
    vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocInfoRes;
    if (vmaCreateBuffer(m_logicalDevice->getAllocator(), &bufferInfo, &vmaAllocInfo, &m_stagingBuffer, &m_stagingAllocation, &allocInfoRes) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create staging ring buffer");
        return false;
    }
    m_mappedData = allocInfoRes.pMappedData;

    VulkanDebugUtils::setObjectName(device, m_stagingBuffer, VK_OBJECT_TYPE_BUFFER, "StagingRingBuffer");

    return true;
}

void VulkanUploadManager::cleanup() {
    auto device = m_logicalDevice->getHandle();
    if (m_uploadContext.uploadFence != VK_NULL_HANDLE) {
        vkDestroyFence(device, m_uploadContext.uploadFence, nullptr);
        m_uploadContext.uploadFence = VK_NULL_HANDLE;
    }
    if (m_uploadContext.commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, m_uploadContext.commandPool, nullptr);
        m_uploadContext.commandPool = VK_NULL_HANDLE;
        m_uploadContext.commandBuffer = VK_NULL_HANDLE;
    }
    if (m_stagingBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_logicalDevice->getAllocator(), m_stagingBuffer, m_stagingAllocation);
        m_stagingBuffer = VK_NULL_HANDLE;
    }
}

VulkanUploadManager::StagingAllocation VulkanUploadManager::allocateStagingBuffer(size_t size) {
    // 256-byte alignment is generally safe for buffer offsets in Vulkan
    size_t alignedSize = (size + 255) & ~255;
    
    if (alignedSize > m_ringSize) {
        spdlog::error("[VULKAN] Upload size {} exceeds ring buffer size {}", size, m_ringSize);
        return {VK_NULL_HANDLE, 0, nullptr};
    }

    if (m_head + alignedSize > m_ringSize) {
        m_head = 0; // Wrap around
    }
    
    // For a true ring buffer we'd need to check if we overwrite m_tail, but since 
    // we currently wait for the fence on each submit, the buffer is always empty when we allocate.
    // To be fully asynchronous we'd use timeline semaphores or multiple fences.
    // For Milestone 6B, we just advance the head.
    
    size_t offset = m_head;
    m_head += alignedSize;
    
    return {
        m_stagingBuffer,
        offset,
        static_cast<uint8_t*>(m_mappedData) + offset
    };
}

void VulkanUploadManager::submitUploads(std::function<void(VkCommandBuffer cmd)>&& function, bool wait) {
    auto device = m_logicalDevice->getHandle();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(m_uploadContext.commandBuffer, &beginInfo);
    
    function(m_uploadContext.commandBuffer);
    
    vkEndCommandBuffer(m_uploadContext.commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_uploadContext.commandBuffer;

    vkQueueSubmit(m_logicalDevice->getGraphicsQueue(), 1, &submitInfo, m_uploadContext.uploadFence);

    if (wait) {
        vkWaitForFences(device, 1, &m_uploadContext.uploadFence, VK_TRUE, 9999999999);
        vkResetFences(device, 1, &m_uploadContext.uploadFence);
        vkResetCommandPool(device, m_uploadContext.commandPool, 0);
        m_head = 0; // Reset ring buffer head since we waited
    }
}

} // namespace luma::render::vulkan
