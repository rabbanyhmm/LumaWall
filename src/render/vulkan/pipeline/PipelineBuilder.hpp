#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <render/vulkan/device/VulkanLogicalDevice.hpp>

namespace luma::render::vulkan {

class PipelineBuilder {
public:
    PipelineBuilder();

    PipelineBuilder& addShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits stage, const char* entryPoint = "main");
    PipelineBuilder& setInputTopology(VkPrimitiveTopology topology);
    PipelineBuilder& setPolygonMode(VkPolygonMode mode);
    PipelineBuilder& setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    PipelineBuilder& disableMultisampling();
    PipelineBuilder& disableBlending();
    PipelineBuilder& enableAlphaBlending();
    PipelineBuilder& setColorAttachmentFormat(VkFormat format);
    PipelineBuilder& setDepthFormat(VkFormat format);
    PipelineBuilder& disableDepthTest();
    PipelineBuilder& setPipelineLayout(VkPipelineLayout layout);

    VkPipeline build(std::shared_ptr<VulkanLogicalDevice> logicalDevice, VkPipelineCache cache = VK_NULL_HANDLE);

private:
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
    VkPipelineInputAssemblyStateCreateInfo m_inputAssembly{};
    VkPipelineRasterizationStateCreateInfo m_rasterizer{};
    VkPipelineColorBlendAttachmentState m_colorBlendAttachment{};
    VkPipelineMultisampleStateCreateInfo m_multisampling{};
    VkPipelineDepthStencilStateCreateInfo m_depthStencil{};
    
    VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
    VkFormat m_colorAttachmentFormat{VK_FORMAT_UNDEFINED};
    VkFormat m_depthAttachmentFormat{VK_FORMAT_UNDEFINED};
};

} // namespace luma::render::vulkan
