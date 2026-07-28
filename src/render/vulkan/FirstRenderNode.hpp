#pragma once
#include <render/common/RenderGraph.hpp>
#include <render/vulkan/command/VulkanContext.hpp>
#include <render/vulkan/texture/VulkanTexture.hpp>
#include <render/vulkan/texture/VulkanTextureUploader.hpp>
#include <render/vulkan/upload/VulkanUploadManager.hpp>
#include <render/vulkan/memory/VulkanExternalMemoryManager.hpp>
#include <media/scheduler/MediaPlayer.hpp>
#include <memory>

namespace luma::render::vulkan {

class FirstRenderNode : public RenderGraphNode {
public:
    struct PushConstants {
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float offsetX = 0.0f;
        float offsetY = 0.0f;
    };

    FirstRenderNode(
        VkPipeline pipeline, 
        VkPipelineLayout pipelineLayout, 
        VkDescriptorSet descriptorSet, 
        PushConstants pc,
        std::shared_ptr<media::MediaPlayer> mediaPlayer = nullptr,
        std::shared_ptr<VulkanTexture> texture = nullptr,
        VulkanUploadManager* uploadManager = nullptr,
        VulkanExternalMemoryManager* extMemManager = nullptr
    ) : m_pipeline(pipeline), 
        m_pipelineLayout(pipelineLayout), 
        m_descriptorSet(descriptorSet), 
        m_pushConstants(pc),
        m_mediaPlayer(std::move(mediaPlayer)),
        m_texture(std::move(texture)),
        m_uploadManager(uploadManager),
        m_extMemManager(extMemManager) {}

    void execute(const RenderFrame& frame) override {
        auto vkContext = dynamic_cast<VulkanContext*>(frame.context);
        if (!vkContext) return;

        // Fetch and upload new video frame if available
        if (m_mediaPlayer && m_texture) {
            auto videoFrame = m_mediaPlayer->getNextFrame();
            if (videoFrame) {
                if (videoFrame->isHardwareBacked()) {
                    VulkanTextureUploader::importHardwareFrame(m_texture, videoFrame, m_extMemManager);
                } else {
                    VulkanTextureUploader::upload(m_texture, videoFrame, m_uploadManager);
                }
            }
        }

        vkContext->beginRenderPass(frame.targetTexture, 0.1f, 0.2f, 0.3f, 1.0f);
        vkContext->bindPipeline(m_pipeline);
        vkContext->bindDescriptorSets(m_pipelineLayout, 0, 1, &m_descriptorSet);
        
        vkCmdPushConstants(vkContext->getHandle(), m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &m_pushConstants);

        // Draw fullscreen triangle (3 vertices)
        vkContext->draw(3, 1, 0, 0);
        
        vkContext->endRenderPass();
    }

private:
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
    VkDescriptorSet m_descriptorSet;
    PushConstants m_pushConstants;

    std::shared_ptr<media::MediaPlayer> m_mediaPlayer;
    std::shared_ptr<VulkanTexture> m_texture;
    VulkanUploadManager* m_uploadManager{nullptr};
    VulkanExternalMemoryManager* m_extMemManager{nullptr};
};

} // namespace luma::render::vulkan
