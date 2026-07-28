#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <render/vulkan/device/VulkanLogicalDevice.hpp>

namespace luma::render::vulkan {

class VulkanPipelineCache {
public:
    VulkanPipelineCache(std::shared_ptr<VulkanLogicalDevice> logicalDevice);
    ~VulkanPipelineCache();

    bool init(const std::string& cacheFilePath = "pipeline_cache.bin");
    void cleanup();

    VkPipelineCache getHandle() const { return m_cache; }

private:
    void saveCacheToDisk();

    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
    VkPipelineCache m_cache{VK_NULL_HANDLE};
    std::string m_cacheFilePath;
};

} // namespace luma::render::vulkan
