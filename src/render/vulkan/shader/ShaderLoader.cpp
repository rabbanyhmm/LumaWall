#include "ShaderLoader.hpp"
#include <core/Logging.hpp>
#include <fstream>
#include <filesystem>

namespace luma::render::vulkan {

ShaderLoader::ShaderLoader(std::shared_ptr<VulkanLogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice)) {
}

ShaderLoader::~ShaderLoader() {
}

VkShaderModule ShaderLoader::loadSpv(const std::string& filepath) {
    auto code = readFile(filepath);
    if (code.empty()) {
        return VK_NULL_HANDLE;
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_logicalDevice->getHandle(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create shader module for {}", filepath);
        return VK_NULL_HANDLE;
    }

    spdlog::info("[VULKAN] Loaded shader module: {}", filepath);
    return shaderModule;
}

void ShaderLoader::destroyModule(VkShaderModule module) {
    if (module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_logicalDevice->getHandle(), module, nullptr);
    }
}

std::vector<uint32_t> ShaderLoader::readFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        spdlog::error("[VULKAN] Failed to open shader file: {}", filepath);
        return {};
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(fileSize));
    file.close();

    return buffer;
}

} // namespace luma::render::vulkan
