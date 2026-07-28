#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <string>
#include <memory>
#include <render/common/ITexture.hpp>
#include <render/vulkan/device/VulkanLogicalDevice.hpp>
#include <render/vulkan/upload/VulkanUploadManager.hpp>
#include <media/common/Frame.hpp>

namespace luma::render::vulkan {

class VulkanTexture : public ITexture {
public:
    VulkanTexture(std::shared_ptr<VulkanLogicalDevice> logicalDevice);
    ~VulkanTexture() override;

    bool loadFromFile(const std::string& filepath, class VulkanUploadManager* uploadManager);
    bool loadFromFrame(std::shared_ptr<media::Frame> frame, VulkanUploadManager* uploadManager);
    
    // YCbCr / Multi-planar support
    bool createYuvImage(uint32_t width, uint32_t height, VkFormat format);
    void updateYuvImage(std::shared_ptr<media::Frame> frame, VulkanUploadManager* uploadManager);

    // Creates a texture directly (useful for swapchain images which don't have VmaAllocation)
    void createFromImage(VkImage image, VkImageView imageView, VkFormat format, uint32_t width, uint32_t height);

    // Creates a texture from an external hardware handle (like DMABUF)
    void createFromExternalMemory(VkImage image, VkDeviceMemory memory, VkImageView imageView, VkSamplerYcbcrConversion conversion, VkSampler sampler, uint32_t width, uint32_t height);

    void destroy();

    uint32_t getWidth() const override { return m_width; }
    uint32_t getHeight() const override { return m_height; }
    TextureFormat getFormat() const override { return TextureFormat::RGBA8; } // Simplified

    VkImage getImage() const { return m_image; }
    VkImageView getImageView() const { return m_imageView; }
    VkSampler getSampler() const { return m_sampler; }
    VkSamplerYcbcrConversion getYcbcrConversion() const { return m_ycbcrConversion; }
    bool isYuv() const { return m_ycbcrConversion != VK_NULL_HANDLE; }

private:
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkCommandBuffer commandBuffer);

    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;

    VkImage m_image{VK_NULL_HANDLE};
    VkImageView m_imageView{VK_NULL_HANDLE};
    VkSampler m_sampler{VK_NULL_HANDLE};
    VmaAllocation m_allocation{VK_NULL_HANDLE};
    VkDeviceMemory m_externalMemory{VK_NULL_HANDLE};

    uint32_t m_width{0};
    uint32_t m_height{0};
    bool m_ownsImage{false}; // True if we created it via VMA

    VkSamplerYcbcrConversion m_ycbcrConversion{VK_NULL_HANDLE};
};

} // namespace luma::render::vulkan
