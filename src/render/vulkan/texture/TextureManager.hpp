#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <render/common/ITexture.hpp>
#include <render/vulkan/texture/VulkanTexture.hpp>
#include <render/vulkan/upload/VulkanUploadManager.hpp>

namespace luma::render::vulkan {

class TextureManager {
public:
    TextureManager(std::shared_ptr<VulkanLogicalDevice> logicalDevice, std::shared_ptr<VulkanUploadManager> uploadManager);
    ~TextureManager();

    std::shared_ptr<ITexture> loadTexture(const std::string& filepath);
    void clearCache();

private:
    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
    std::shared_ptr<VulkanUploadManager> m_uploadManager;
    std::unordered_map<std::string, std::shared_ptr<VulkanTexture>> m_textures;
};

} // namespace luma::render::vulkan
