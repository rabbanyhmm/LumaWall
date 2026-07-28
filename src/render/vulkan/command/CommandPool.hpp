#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <render/vulkan/device/VulkanLogicalDevice.hpp>

namespace luma::render::vulkan {

class CommandPool {
public:
    CommandPool(std::shared_ptr<VulkanLogicalDevice> logicalDevice, uint32_t queueFamilyIndex);
    ~CommandPool();

    bool init();
    void cleanup();

    VkCommandBuffer allocateBuffer();
    void freeBuffer(VkCommandBuffer buffer);
    
    VkCommandPool getHandle() const { return m_pool; }

private:
    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
    uint32_t m_queueFamilyIndex;
    VkCommandPool m_pool{VK_NULL_HANDLE};
};

} // namespace luma::render::vulkan
