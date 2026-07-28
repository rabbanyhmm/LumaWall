#include "PipelineBuilder.hpp"
#include <core/Logging.hpp>

namespace luma::render::vulkan {

PipelineBuilder::PipelineBuilder() {
    m_inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    
    m_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_rasterizer.depthClampEnable = VK_FALSE;
    m_rasterizer.rasterizerDiscardEnable = VK_FALSE;
    m_rasterizer.lineWidth = 1.0f;

    m_multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    
    m_depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    
    m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
}

PipelineBuilder& PipelineBuilder::addShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits stage, const char* entryPoint) {
    VkPipelineShaderStageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage = stage;
    createInfo.module = shaderModule;
    createInfo.pName = entryPoint;
    m_shaderStages.push_back(createInfo);
    return *this;
}

PipelineBuilder& PipelineBuilder::setInputTopology(VkPrimitiveTopology topology) {
    m_inputAssembly.topology = topology;
    m_inputAssembly.primitiveRestartEnable = VK_FALSE;
    return *this;
}

PipelineBuilder& PipelineBuilder::setPolygonMode(VkPolygonMode mode) {
    m_rasterizer.polygonMode = mode;
    return *this;
}

PipelineBuilder& PipelineBuilder::setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace) {
    m_rasterizer.cullMode = cullMode;
    m_rasterizer.frontFace = frontFace;
    return *this;
}

PipelineBuilder& PipelineBuilder::disableMultisampling() {
    m_multisampling.sampleShadingEnable = VK_FALSE;
    m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    return *this;
}

PipelineBuilder& PipelineBuilder::disableBlending() {
    m_colorBlendAttachment.blendEnable = VK_FALSE;
    return *this;
}

PipelineBuilder& PipelineBuilder::enableAlphaBlending() {
    m_colorBlendAttachment.blendEnable = VK_TRUE;
    m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    return *this;
}

PipelineBuilder& PipelineBuilder::setColorAttachmentFormat(VkFormat format) {
    m_colorAttachmentFormat = format;
    return *this;
}

PipelineBuilder& PipelineBuilder::setDepthFormat(VkFormat format) {
    m_depthAttachmentFormat = format;
    return *this;
}

PipelineBuilder& PipelineBuilder::disableDepthTest() {
    m_depthStencil.depthTestEnable = VK_FALSE;
    m_depthStencil.depthWriteEnable = VK_FALSE;
    m_depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    m_depthStencil.depthBoundsTestEnable = VK_FALSE;
    m_depthStencil.stencilTestEnable = VK_FALSE;
    return *this;
}

PipelineBuilder& PipelineBuilder::setPipelineLayout(VkPipelineLayout layout) {
    m_pipelineLayout = layout;
    return *this;
}

VkPipeline PipelineBuilder::build(std::shared_ptr<VulkanLogicalDevice> logicalDevice, VkPipelineCache cache) {
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &m_colorBlendAttachment;

    // Use empty vertex input state for fullscreen triangle generating shaders
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // Dynamic states for viewport and scissor
    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Vulkan 1.3 Dynamic Rendering
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    if (m_colorAttachmentFormat != VK_FORMAT_UNDEFINED) {
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = &m_colorAttachmentFormat;
    }
    renderingInfo.depthAttachmentFormat = m_depthAttachmentFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
    pipelineInfo.pStages = m_shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &m_inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &m_rasterizer;
    pipelineInfo.pMultisampleState = &m_multisampling;
    pipelineInfo.pDepthStencilState = &m_depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    
    // We are not using legacy render passes
    pipelineInfo.renderPass = VK_NULL_HANDLE;

    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(logicalDevice->getHandle(), cache, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create graphics pipeline");
        return VK_NULL_HANDLE;
    }

    return newPipeline;
}

} // namespace luma::render::vulkan
