#include "TextureManager.hpp"
#include <core/Logging.hpp>

namespace luma::render::vulkan {

TextureManager::TextureManager(std::shared_ptr<VulkanLogicalDevice> logicalDevice, std::shared_ptr<VulkanUploadManager> uploadManager)
    : m_logicalDevice(std::move(logicalDevice)), m_uploadManager(std::move(uploadManager)) {
}

TextureManager::~TextureManager() {
    clearCache();
}

std::shared_ptr<ITexture> TextureManager::loadTexture(const std::string& filepath) {
    if (m_textures.find(filepath) != m_textures.end()) {
        return m_textures[filepath];
    }

    auto texture = std::make_shared<VulkanTexture>(m_logicalDevice);
    if (!texture->loadFromFile(filepath, m_uploadManager.get())) {
        spdlog::error("[VULKAN] TextureManager failed to load {}", filepath);
        return nullptr;
    }

    m_textures[filepath] = texture;
    spdlog::info("[VULKAN] Loaded and cached texture: {}", filepath);
    return texture;
}

void TextureManager::clearCache() {
    m_textures.clear();
}

} // namespace luma::render::vulkan
