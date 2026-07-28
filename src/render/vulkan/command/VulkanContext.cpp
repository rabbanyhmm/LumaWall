#include "VulkanContext.hpp"
#include <core/Logging.hpp>
#include <render/vulkan/texture/VulkanTexture.hpp>
#include <media/common/MediaTelemetry.hpp>

namespace luma::render::vulkan {

VulkanContext::VulkanContext(std::shared_ptr<VulkanLogicalDevice> logicalDevice, std::shared_ptr<CommandPool> commandPool)
    : m_logicalDevice(std::move(logicalDevice)), m_commandPool(std::move(commandPool)) {
}

VulkanContext::~VulkanContext() {
    m_frames.clear();
}

bool VulkanContext::init(uint32_t maxFramesInFlight) {
    m_frames.clear();    
    m_gpuTimer = std::make_shared<GPUTimer>(m_logicalDevice);

    for (uint32_t i = 0; i < maxFramesInFlight; i++) {
        auto frame = std::make_unique<FrameContext>(m_logicalDevice, m_commandPool, i);
        if (!frame->init()) {
            return false;
        }
        m_frames.push_back(std::move(frame));
    }
    return true;
}

FrameContext* VulkanContext::getCurrentFrameContext() const {
    if (m_frames.empty()) return nullptr;
    return m_frames[m_currentFrameIndex].get();
}

void VulkanContext::advanceFrame() {
    if (!m_frames.empty()) {
        m_currentFrameIndex = (m_currentFrameIndex + 1) % m_frames.size();
    }
}

void VulkanContext::begin(const RenderFrame& frame) {
    auto frameContext = getCurrentFrameContext();
    if (!frameContext) return;
    
    // resetFence is usually handled in acquireNextFrame or here if we know the frame is safe
    // Actually, wait for fence before resetting it to ensure GPU is done with this frame context
    frameContext->waitForFence();
    frameContext->resetFence();
    // Resolve timings from this frame's PREVIOUS execution
    m_gpuTimer->resolve(frameContext);
    float renderTime = m_gpuTimer->getTimeMS("RenderGraph");
    if (renderTime > 0.0f) {
        media::MediaTelemetry::getInstance().recordGpuRenderTime(renderTime);
        // spdlog::info("[GPU] RenderGraph: {:.3f} ms", renderTime); // Disable excessive logging for now
    }

    // Reset command buffer and query pool
    vkResetCommandBuffer(frameContext->getCommandBuffer(), 0);
    m_gpuTimer->reset(frameContext);
    
    m_commandBuffer = frameContext->getCommandBuffer();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(m_commandBuffer, &beginInfo) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to begin recording command buffer");
    }

    m_gpuTimer->startRecord(frameContext, "RenderGraph");
}

void VulkanContext::end() {
    auto frameContext = getCurrentFrameContext();
    if (frameContext) {
        m_gpuTimer->endRecord(frameContext, "RenderGraph");
    }

    if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to record command buffer");
    }
}

// Removed clear()

void VulkanContext::submit(const RenderFrame& frame) {
    auto frameContext = getCurrentFrameContext();
    if (!frameContext) return;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {frameContext->getImageAvailableSemaphore()};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    VkSemaphore signalSemaphores[] = {frameContext->getRenderFinishedSemaphore()};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_logicalDevice->getGraphicsQueue(), 1, &submitInfo, frameContext->getInFlightFence()) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to submit draw command buffer");
    }
}
void VulkanContext::bindShader(IShader*) {}
void VulkanContext::bindTexture(uint32_t, ITexture*) {}
void VulkanContext::bindVertexBuffer(IBuffer*) {}
void VulkanContext::bindIndexBuffer(IBuffer*) {}
void VulkanContext::draw(uint32_t, uint32_t) {}
void VulkanContext::drawIndexed(uint32_t, uint32_t) {}
void VulkanContext::beginRenderPass(ITexture* renderTarget, float r, float g, float b, float a) {
    if (!renderTarget) return;

    auto vkTexture = dynamic_cast<VulkanTexture*>(renderTarget);
    if (!vkTexture) return;

    // Transition image to COLOR_ATTACHMENT_OPTIMAL
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vkTexture->getImage();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(
        m_commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
    
    // Store texture for endRenderPass
    m_currentRenderTarget = vkTexture;

    VkClearValue clearValue{};
    clearValue.color = {{r, g, b, a}};

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = vkTexture->getImageView();
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clearValue;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = { renderTarget->getWidth(), renderTarget->getHeight() };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(m_commandBuffer, &renderingInfo);

    // Set dynamic viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(renderTarget->getWidth());
    viewport.height = static_cast<float>(renderTarget->getHeight());
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = renderingInfo.renderArea.extent;
    vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
}

void VulkanContext::endRenderPass() {
    vkCmdEndRendering(m_commandBuffer);
    
    if (m_currentRenderTarget) {
        // Transition image to PRESENT_SRC_KHR
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_currentRenderTarget->getImage();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(
            m_commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
        m_currentRenderTarget = nullptr;
    }
}

void VulkanContext::bindPipeline(VkPipeline pipeline) {
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void VulkanContext::bindDescriptorSets(VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* descriptorSets) {
    vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, firstSet, descriptorSetCount, descriptorSets, 0, nullptr);
}

void VulkanContext::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

} // namespace luma::render::vulkan
