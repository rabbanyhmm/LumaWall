#include "DescriptorBuilder.hpp"
#include <core/Logging.hpp>

namespace luma::render::vulkan {

DescriptorBuilder DescriptorBuilder::begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator) {
    DescriptorBuilder builder;
    builder.m_cache = layoutCache;
    builder.m_allocator = allocator;
    return builder;
}

DescriptorBuilder& DescriptorBuilder::bindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags) {
    VkDescriptorSetLayoutBinding newBinding{};
    newBinding.descriptorCount = 1;
    newBinding.descriptorType = type;
    newBinding.pImmutableSamplers = nullptr;
    newBinding.stageFlags = stageFlags;
    newBinding.binding = binding;
    
    m_bindings.push_back(newBinding);

    VkWriteDescriptorSet newWrite{};
    newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    newWrite.pNext = nullptr;
    newWrite.descriptorCount = 1;
    newWrite.descriptorType = type;
    newWrite.pBufferInfo = bufferInfo;
    newWrite.dstBinding = binding;

    m_writes.push_back(newWrite);
    return *this;
}

DescriptorBuilder& DescriptorBuilder::bindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags) {
    VkDescriptorSetLayoutBinding newBinding{};
    newBinding.descriptorCount = 1;
    newBinding.descriptorType = type;
    newBinding.pImmutableSamplers = nullptr;
    newBinding.stageFlags = stageFlags;
    newBinding.binding = binding;
    
    m_bindings.push_back(newBinding);

    VkWriteDescriptorSet newWrite{};
    newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    newWrite.pNext = nullptr;
    newWrite.descriptorCount = 1;
    newWrite.descriptorType = type;
    newWrite.pImageInfo = imageInfo;
    newWrite.dstBinding = binding;

    m_writes.push_back(newWrite);
    return *this;
}

DescriptorBuilder& DescriptorBuilder::bindImageYcbcr(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags, const VkSampler* immutableSampler) {
    VkDescriptorSetLayoutBinding newBinding{};
    newBinding.descriptorCount = 1;
    newBinding.descriptorType = type;
    newBinding.pImmutableSamplers = immutableSampler;
    newBinding.stageFlags = stageFlags;
    newBinding.binding = binding;
    
    m_bindings.push_back(newBinding);

    VkWriteDescriptorSet newWrite{};
    newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    newWrite.pNext = nullptr;
    newWrite.descriptorCount = 1;
    newWrite.descriptorType = type;
    newWrite.pImageInfo = imageInfo;
    newWrite.dstBinding = binding;

    m_writes.push_back(newWrite);
    return *this;
}

bool DescriptorBuilder::build(VkDescriptorSet& set, VkDescriptorSetLayout& layout) {
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = nullptr;
    layoutInfo.pBindings = m_bindings.data();
    layoutInfo.bindingCount = static_cast<uint32_t>(m_bindings.size());

    layout = m_cache->createLayout(&layoutInfo);
    
    if (!m_allocator->allocate(&set, layout)) {
        return false;
    }

    for (auto& w : m_writes) {
        w.dstSet = set;
    }

    vkUpdateDescriptorSets(m_allocator->getDevice(), static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0, nullptr);

    return true;
}

bool DescriptorBuilder::build(VkDescriptorSet& set) {
    VkDescriptorSetLayout layout;
    return build(set, layout);
}

} // namespace luma::render::vulkan
