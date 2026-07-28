#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <render/vulkan/descriptor/DescriptorLayoutCache.hpp>
#include <render/vulkan/descriptor/DescriptorAllocator.hpp>

namespace luma::render::vulkan {

class DescriptorBuilder {
public:
    static DescriptorBuilder begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator);

    DescriptorBuilder& bindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
    DescriptorBuilder& bindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
    DescriptorBuilder& bindImageYcbcr(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags, const VkSampler* immutableSampler);

    bool build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
    bool build(VkDescriptorSet& set);

private:
    std::vector<VkWriteDescriptorSet> m_writes;
    std::vector<VkDescriptorSetLayoutBinding> m_bindings;

    DescriptorLayoutCache* m_cache;
    DescriptorAllocator* m_allocator;
};

} // namespace luma::render::vulkan
