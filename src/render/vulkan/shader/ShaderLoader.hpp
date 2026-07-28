#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>
#include <render/vulkan/device/VulkanLogicalDevice.hpp>

namespace luma::render::vulkan {

class ShaderLoader {
public:
    ShaderLoader(std::shared_ptr<VulkanLogicalDevice> logicalDevice);
    ~ShaderLoader();

    VkShaderModule loadSpv(const std::string& filepath);
    void destroyModule(VkShaderModule module);

private:
    std::vector<uint32_t> readFile(const std::string& filepath);

    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
};

} // namespace luma::render::vulkan
