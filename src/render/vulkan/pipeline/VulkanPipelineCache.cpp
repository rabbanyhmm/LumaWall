#include "VulkanPipelineCache.hpp"
#include <core/Logging.hpp>
#include <fstream>
#include <vector>

namespace luma::render::vulkan {

VulkanPipelineCache::VulkanPipelineCache(std::shared_ptr<VulkanLogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice)) {
}

VulkanPipelineCache::~VulkanPipelineCache() {
    cleanup();
}

bool VulkanPipelineCache::init(const std::string& cacheFilePath) {
    m_cacheFilePath = cacheFilePath;
    
    std::vector<char> cacheData;
    std::ifstream file(m_cacheFilePath, std::ios::binary | std::ios::ate);
    
    if (file.is_open()) {
        size_t fileSize = static_cast<size_t>(file.tellg());
        cacheData.resize(fileSize);
        file.seekg(0);
        file.read(cacheData.data(), static_cast<std::streamsize>(fileSize));
        file.close();
        spdlog::info("[VULKAN] Loaded pipeline cache from {}", m_cacheFilePath);
    } else {
        spdlog::info("[VULKAN] No pipeline cache found at {}, starting fresh", m_cacheFilePath);
    }

    VkPipelineCacheCreateInfo cacheInfo{};
    cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    cacheInfo.initialDataSize = cacheData.size();
    cacheInfo.pInitialData = cacheData.data();

    if (vkCreatePipelineCache(m_logicalDevice->getHandle(), &cacheInfo, nullptr, &m_cache) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create pipeline cache");
        return false;
    }

    return true;
}

void VulkanPipelineCache::saveCacheToDisk() {
    if (m_cache == VK_NULL_HANDLE) return;

    size_t dataSize = 0;
    if (vkGetPipelineCacheData(m_logicalDevice->getHandle(), m_cache, &dataSize, nullptr) != VK_SUCCESS) {
        spdlog::warn("[VULKAN] Failed to get pipeline cache size");
        return;
    }

    std::vector<char> cacheData(dataSize);
    if (vkGetPipelineCacheData(m_logicalDevice->getHandle(), m_cache, &dataSize, cacheData.data()) != VK_SUCCESS) {
        spdlog::warn("[VULKAN] Failed to get pipeline cache data");
        return;
    }

    std::ofstream file(m_cacheFilePath, std::ios::binary);
    if (file.is_open()) {
        file.write(cacheData.data(), static_cast<std::streamsize>(dataSize));
        file.close();
        spdlog::info("[VULKAN] Saved pipeline cache to {}", m_cacheFilePath);
    } else {
        spdlog::warn("[VULKAN] Failed to save pipeline cache to {}", m_cacheFilePath);
    }
}

void VulkanPipelineCache::cleanup() {
    if (m_cache != VK_NULL_HANDLE) {
        saveCacheToDisk();
        vkDestroyPipelineCache(m_logicalDevice->getHandle(), m_cache, nullptr);
        m_cache = VK_NULL_HANDLE;
    }
}

} // namespace luma::render::vulkan
