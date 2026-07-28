#include "DescriptorLayoutCache.hpp"
#include <core/Logging.hpp>

namespace luma::render::vulkan {

DescriptorLayoutCache::DescriptorLayoutCache(std::shared_ptr<VulkanLogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice)) {
}

DescriptorLayoutCache::~DescriptorLayoutCache() {
    cleanup();
}

void DescriptorLayoutCache::cleanup() {
    auto device = m_logicalDevice->getHandle();
    for (auto pair : m_layoutCache) {
        vkDestroyDescriptorSetLayout(device, pair.second, nullptr);
    }
    m_layoutCache.clear();
}

VkDescriptorSetLayout DescriptorLayoutCache::createLayout(VkDescriptorSetLayoutCreateInfo* info) {
    DescriptorLayoutInfo layoutInfo;
    layoutInfo.bindings.reserve(info->bindingCount);
    bool isSorted = true;
    uint32_t lastBinding = static_cast<uint32_t>(-1);

    for (uint32_t i = 0; i < info->bindingCount; i++) {
        layoutInfo.bindings.push_back(info->pBindings[i]);
        if (lastBinding == static_cast<uint32_t>(-1) || info->pBindings[i].binding > lastBinding) {
            lastBinding = info->pBindings[i].binding;
        } else {
            isSorted = false;
        }
    }

    if (!isSorted) {
        std::sort(layoutInfo.bindings.begin(), layoutInfo.bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b) {
            return a.binding < b.binding;
        });
    }

    auto it = m_layoutCache.find(layoutInfo);
    if (it != m_layoutCache.end()) {
        return it->second;
    }

    VkDescriptorSetLayout layout;
    if (vkCreateDescriptorSetLayout(m_logicalDevice->getHandle(), info, nullptr, &layout) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create descriptor set layout");
        return VK_NULL_HANDLE;
    }

    m_layoutCache[layoutInfo] = layout;
    return layout;
}

bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const {
    if (other.bindings.size() != bindings.size()) {
        return false;
    }
    for (size_t i = 0; i < bindings.size(); i++) {
        if (other.bindings[i].binding != bindings[i].binding ||
            other.bindings[i].descriptorType != bindings[i].descriptorType ||
            other.bindings[i].descriptorCount != bindings[i].descriptorCount ||
            other.bindings[i].stageFlags != bindings[i].stageFlags) {
            return false;
        }
    }
    return true;
}

size_t DescriptorLayoutCache::DescriptorLayoutInfo::hash() const {
    size_t result = std::hash<size_t>()(bindings.size());
    for (const auto& b : bindings) {
        size_t binding_hash = static_cast<size_t>(b.binding) | (static_cast<size_t>(b.descriptorType) << 8) | (static_cast<size_t>(b.descriptorCount) << 16) | (static_cast<size_t>(b.stageFlags) << 24);
        result ^= std::hash<size_t>()(binding_hash);
    }
    return result;
}

} // namespace luma::render::vulkan
