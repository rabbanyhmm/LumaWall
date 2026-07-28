#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <render/vulkan/device/VulkanLogicalDevice.hpp>

namespace luma::render::vulkan {

class DescriptorLayoutCache {
public:
    DescriptorLayoutCache(std::shared_ptr<VulkanLogicalDevice> logicalDevice);
    ~DescriptorLayoutCache();

    void cleanup();

    VkDescriptorSetLayout createLayout(VkDescriptorSetLayoutCreateInfo* info);

private:
    struct DescriptorLayoutInfo {
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        bool operator==(const DescriptorLayoutInfo& other) const;
        size_t hash() const;
    };

    struct DescriptorLayoutHash {
        std::size_t operator()(const DescriptorLayoutInfo& k) const {
            return k.hash();
        }
    };

    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
    std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> m_layoutCache;
};

} // namespace luma::render::vulkan
