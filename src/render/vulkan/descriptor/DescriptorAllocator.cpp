#include "DescriptorAllocator.hpp"
#include <core/Logging.hpp>

namespace luma::render::vulkan {

DescriptorAllocator::DescriptorAllocator(std::shared_ptr<VulkanLogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice)) {
}

DescriptorAllocator::~DescriptorAllocator() {
    cleanup();
}

void DescriptorAllocator::cleanup() {
    auto device = m_logicalDevice->getHandle();
    for (auto pool : m_usedPools) {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
    for (auto pool : m_freePools) {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
    m_usedPools.clear();
    m_freePools.clear();
    m_currentPool = VK_NULL_HANDLE;
}

VkDevice DescriptorAllocator::getDevice() const {
    return m_logicalDevice->getHandle();
}

void DescriptorAllocator::resetPools() {
    auto device = m_logicalDevice->getHandle();
    for (auto pool : m_usedPools) {
        vkResetDescriptorPool(device, pool, 0);
        m_freePools.push_back(pool);
    }
    m_usedPools.clear();
    m_currentPool = VK_NULL_HANDLE;
}

bool DescriptorAllocator::allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout) {
    if (m_currentPool == VK_NULL_HANDLE) {
        m_currentPool = getPool();
        m_usedPools.push_back(m_currentPool);
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_currentPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkResult result = vkAllocateDescriptorSets(m_logicalDevice->getHandle(), &allocInfo, set);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        m_currentPool = getPool();
        m_usedPools.push_back(m_currentPool);

        allocInfo.descriptorPool = m_currentPool;
        result = vkAllocateDescriptorSets(m_logicalDevice->getHandle(), &allocInfo, set);
    }

    if (result != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to allocate descriptor set");
        return false;
    }
    return true;
}

VkDescriptorPool DescriptorAllocator::getPool() {
    if (!m_freePools.empty()) {
        VkDescriptorPool pool = m_freePools.back();
        m_freePools.pop_back();
        return pool;
    }
    return createPool(1000, 0);
}

VkDescriptorPool DescriptorAllocator::createPool(uint32_t count, VkDescriptorPoolCreateFlags flags) {
    std::vector<VkDescriptorPoolSize> sizes = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, count },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, count },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, count },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count },
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = flags;
    poolInfo.maxSets = count;
    poolInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
    poolInfo.pPoolSizes = sizes.data();

    VkDescriptorPool pool;
    if (vkCreateDescriptorPool(m_logicalDevice->getHandle(), &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create descriptor pool");
        return VK_NULL_HANDLE;
    }
    return pool;
}

} // namespace luma::render::vulkan
