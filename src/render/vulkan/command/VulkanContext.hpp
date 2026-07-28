#pragma once
#include <vulkan/vulkan.h>
#include <render/common/IRenderContext.hpp>
#include <render/common/RenderFrame.hpp>
#include <render/vulkan/command/CommandPool.hpp>
#include <render/vulkan/frame/FrameContext.hpp>
#include <render/vulkan/debug/GPUTimer.hpp>
#include <memory>
#include <vector>

namespace luma::render::vulkan {

class VulkanContext : public IRenderContext {
public:
    VulkanContext(std::shared_ptr<VulkanLogicalDevice> logicalDevice, std::shared_ptr<CommandPool> commandPool);
    ~VulkanContext() override;
    
    bool init(uint32_t maxFramesInFlight = 2);

    void begin(const RenderFrame& frame) override;
    void end() override;
    void submit(const RenderFrame& frame) override;

    void bindShader(IShader* shader) override;
    void bindTexture(uint32_t slot, ITexture* texture) override;
    void bindVertexBuffer(IBuffer* buffer) override;
    void bindIndexBuffer(IBuffer* buffer) override;
    
    void draw(uint32_t vertexCount, uint32_t instanceCount) override;
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount) override;

    void beginRenderPass(ITexture* renderTarget, float r=0.f, float g=0.f, float b=0.f, float a=1.f) override;
    void endRenderPass() override;
    
    // Vulkan specific
    void bindPipeline(VkPipeline pipeline);
    void bindDescriptorSets(VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* descriptorSets);
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);

    VkCommandBuffer getHandle() const { return m_commandBuffer; }
    
    // Will be set by RenderScheduler or during beginRenderPass
    void setCommandBuffer(VkCommandBuffer cmd) { m_commandBuffer = cmd; }

    FrameContext* getCurrentFrameContext() const;
    void advanceFrame();

    std::shared_ptr<GPUTimer> getGPUTimer() { return m_gpuTimer; }

private:
    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
    std::shared_ptr<CommandPool> m_commandPool;
    std::shared_ptr<GPUTimer> m_gpuTimer;
    std::vector<std::unique_ptr<FrameContext>> m_frames;
    uint32_t m_currentFrameIndex = 0;
    
    VkCommandBuffer m_commandBuffer{VK_NULL_HANDLE};
    class VulkanTexture* m_currentRenderTarget{nullptr};
};

} // namespace luma::render::vulkan
