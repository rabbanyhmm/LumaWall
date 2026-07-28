#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <render/vulkan/device/VulkanLogicalDevice.hpp>

namespace luma::render::vulkan {

class DescriptorAllocator {
public:
    DescriptorAllocator(std::shared_ptr<VulkanLogicalDevice> logicalDevice);
    ~DescriptorAllocator();

    void resetPools();
    bool allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);
    void cleanup();

    VkDevice getDevice() const;

private:
    VkDescriptorPool createPool(uint32_t count, VkDescriptorPoolCreateFlags flags);
    VkDescriptorPool getPool();

    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
    VkDescriptorPool m_currentPool{VK_NULL_HANDLE};
    std::vector<VkDescriptorPool> m_usedPools;
    std::vector<VkDescriptorPool> m_freePools;
};

} // namespace luma::render::vulkan
