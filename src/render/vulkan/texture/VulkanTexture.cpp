#include "VulkanTexture.hpp"
#include <core/Logging.hpp>
#include <render/vulkan/debug/VulkanDebugUtils.hpp>
#include <render/vulkan/upload/VulkanUploadManager.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace luma::render::vulkan {

VulkanTexture::VulkanTexture(std::shared_ptr<VulkanLogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice)) {
}

VulkanTexture::~VulkanTexture() {
    destroy();
}

void VulkanTexture::destroy() {
    auto device = m_logicalDevice->getHandle();

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    if (m_ycbcrConversion != VK_NULL_HANDLE) {
        vkDestroySamplerYcbcrConversion(device, m_ycbcrConversion, nullptr);
        m_ycbcrConversion = VK_NULL_HANDLE;
    }

    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }

    if (m_ownsImage && m_image != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE) {
        vmaDestroyImage(m_logicalDevice->getAllocator(), m_image, m_allocation);
        m_image = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
    }

    if (m_externalMemory != VK_NULL_HANDLE) {
        if (m_image != VK_NULL_HANDLE) {
            vkDestroyImage(device, m_image, nullptr);
            m_image = VK_NULL_HANDLE;
        }
        vkFreeMemory(device, m_externalMemory, nullptr);
        m_externalMemory = VK_NULL_HANDLE;
    }
}

void VulkanTexture::createFromImage(VkImage image, VkImageView imageView, VkFormat format, uint32_t width, uint32_t height) {
    destroy(); // clean up if already loaded
    
    m_image = image;
    m_imageView = imageView;
    m_width = width;
    m_height = height;
    m_ownsImage = false;
    
    // We don't create a sampler here. This is typically used for Swapchain images which don't need sampling.
}

void VulkanTexture::createFromExternalMemory(VkImage image, VkDeviceMemory memory, VkImageView imageView, VkSamplerYcbcrConversion conversion, VkSampler sampler, uint32_t width, uint32_t height) {
    destroy();
    
    m_image = image;
    m_externalMemory = memory;
    m_imageView = imageView;
    m_ycbcrConversion = conversion;
    m_sampler = sampler;
    m_width = width;
    m_height = height;
    m_ownsImage = false; // We own it manually via m_externalMemory
}

bool VulkanTexture::loadFromFile(const std::string& filepath, VulkanUploadManager* uploadManager) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        spdlog::error("[VULKAN] Failed to load image file {}", filepath);
        return false;
    }

    m_width = static_cast<uint32_t>(texWidth);
    m_height = static_cast<uint32_t>(texHeight);

    // Allocate from ring buffer
    auto stagingAlloc = uploadManager->allocateStagingBuffer(imageSize);
    if (stagingAlloc.buffer == VK_NULL_HANDLE) {
        stbi_image_free(pixels);
        return false;
    }

    memcpy(stagingAlloc.mappedData, pixels, static_cast<size_t>(imageSize));
    stbi_image_free(pixels);

    // Create Image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_width;
    imageInfo.extent.height = m_height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(m_logicalDevice->getAllocator(), &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr);
    m_ownsImage = true;

    // Transition & Copy using UploadManager
    uploadManager->submitUploads([&](VkCommandBuffer cmd) {
        transitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmd);
        
        // Inline copy buffer to image
        VkBufferImageCopy region{};
        region.bufferOffset = stagingAlloc.offset; // Using ring buffer offset
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = { m_width, m_height, 1 };

        vkCmdCopyBufferToImage(cmd, stagingAlloc.buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        transitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmd);
    }, true); // wait = true for synchronous load

    // Create ImageView
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vkCreateImageView(m_logicalDevice->getHandle(), &viewInfo, nullptr, &m_imageView);

    // Create Sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    vkCreateSampler(m_logicalDevice->getHandle(), &samplerInfo, nullptr, &m_sampler);

    // Debug names
    VulkanDebugUtils::setObjectName(m_logicalDevice->getHandle(), m_image, VK_OBJECT_TYPE_IMAGE, "Texture_" + filepath);
    VulkanDebugUtils::setObjectName(m_logicalDevice->getHandle(), m_imageView, VK_OBJECT_TYPE_IMAGE_VIEW, "TextureView_" + filepath);

    return true;
}

bool VulkanTexture::loadFromFrame(std::shared_ptr<media::Frame> frame, VulkanUploadManager* uploadManager) {
    if (!frame || !frame->isValid()) return false;

    m_width = frame->width;
    m_height = frame->height;

    size_t imageSize = m_width * m_height * 4; // Assuming RGBA8
    if (frame->data.size() != imageSize) {
        spdlog::error("[VULKAN] Frame data size mismatch");
        return false;
    }

    auto stagingAlloc = uploadManager->allocateStagingBuffer(imageSize);
    if (!stagingAlloc.buffer) {
        spdlog::error("[VULKAN] Failed to allocate staging buffer for frame");
        return false;
    }

    memcpy(stagingAlloc.mappedData, frame->data.data(), imageSize);

    // Create Image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_width;
    imageInfo.extent.height = m_height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(m_logicalDevice->getAllocator(), &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr);
    m_ownsImage = true;

    // Transition & Copy using UploadManager
    uploadManager->submitUploads([&](VkCommandBuffer cmd) {
        transitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmd);
        
        // Inline copy buffer to image
        VkBufferImageCopy region{};
        region.bufferOffset = stagingAlloc.offset; // Using ring buffer offset
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = { m_width, m_height, 1 };

        vkCmdCopyBufferToImage(cmd, stagingAlloc.buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        transitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmd);
    }, true); // wait = true for synchronous load

    // Create ImageView
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vkCreateImageView(m_logicalDevice->getHandle(), &viewInfo, nullptr, &m_imageView);

    // Create Sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    vkCreateSampler(m_logicalDevice->getHandle(), &samplerInfo, nullptr, &m_sampler);

    VulkanDebugUtils::setObjectName(m_logicalDevice->getHandle(), m_image, VK_OBJECT_TYPE_IMAGE, "Texture_Frame");
    VulkanDebugUtils::setObjectName(m_logicalDevice->getHandle(), m_imageView, VK_OBJECT_TYPE_IMAGE_VIEW, "TextureView_Frame");

    return true;
}

bool VulkanTexture::createYuvImage(uint32_t width, uint32_t height, VkFormat format) {
    destroy();
    m_width = width;
    m_height = height;

    // Create YCbCr Conversion
    VkSamplerYcbcrConversionCreateInfo ycbcrInfo{};
    ycbcrInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO;
    ycbcrInfo.format = format;
    ycbcrInfo.ycbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709;
    ycbcrInfo.ycbcrRange = VK_SAMPLER_YCBCR_RANGE_ITU_NARROW; // Standard video range
    ycbcrInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    ycbcrInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    ycbcrInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    ycbcrInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    ycbcrInfo.xChromaOffset = VK_CHROMA_LOCATION_MIDPOINT;
    ycbcrInfo.yChromaOffset = VK_CHROMA_LOCATION_MIDPOINT;
    ycbcrInfo.chromaFilter = VK_FILTER_LINEAR;
    ycbcrInfo.forceExplicitReconstruction = VK_FALSE;

    if (vkCreateSamplerYcbcrConversion(m_logicalDevice->getHandle(), &ycbcrInfo, nullptr, &m_ycbcrConversion) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create YCbCr conversion");
        return false;
    }

    // Create Image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_width;
    imageInfo.extent.height = m_height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    if (vmaCreateImage(m_logicalDevice->getAllocator(), &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr) != VK_SUCCESS) {
        return false;
    }
    m_ownsImage = true;

    // Create ImageView
    VkSamplerYcbcrConversionInfo samplerYcbcrInfo{};
    samplerYcbcrInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO;
    samplerYcbcrInfo.conversion = m_ycbcrConversion;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = &samplerYcbcrInfo;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_logicalDevice->getHandle(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        return false;
    }

    // Create Sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.pNext = &samplerYcbcrInfo;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(m_logicalDevice->getHandle(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        return false;
    }

    return true;
}

void VulkanTexture::updateYuvImage(std::shared_ptr<media::Frame> frame, VulkanUploadManager* uploadManager) {
    if (!frame || !frame->isValid() || !m_image) return;

    size_t ySize = frame->data.size();
    size_t uvSize1 = frame->dataPlane1.size();
    size_t uvSize2 = frame->dataPlane2.size();
    size_t totalSize = ySize + uvSize1 + uvSize2;

    auto stagingAlloc = uploadManager->allocateStagingBuffer(totalSize);
    if (!stagingAlloc.buffer) return;

    uint8_t* mapped = static_cast<uint8_t*>(stagingAlloc.mappedData);
    memcpy(mapped, frame->data.data(), ySize);
    if (uvSize1 > 0) memcpy(mapped + ySize, frame->dataPlane1.data(), uvSize1);
    if (uvSize2 > 0) memcpy(mapped + ySize + uvSize1, frame->dataPlane2.data(), uvSize2);

    uploadManager->submitUploads([&](VkCommandBuffer cmd) {
        // Transition Undefined -> Transfer Dst (For multi-planar, barrier on COLOR_BIT applies to all planes)
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        // Copy Plane 0 (Y)
        VkBufferImageCopy regionY{};
        regionY.bufferOffset = stagingAlloc.offset;
        regionY.imageSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
        regionY.imageSubresource.layerCount = 1;
        regionY.imageExtent = { m_width, m_height, 1 };
        vkCmdCopyBufferToImage(cmd, stagingAlloc.buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regionY);

        if (uvSize1 > 0 && uvSize2 == 0) {
            // NV12 - Plane 1 (UV interleaved)
            VkBufferImageCopy regionUV{};
            regionUV.bufferOffset = stagingAlloc.offset + ySize;
            regionUV.imageSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
            regionUV.imageSubresource.layerCount = 1;
            regionUV.imageExtent = { m_width / 2, m_height / 2, 1 };
            vkCmdCopyBufferToImage(cmd, stagingAlloc.buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regionUV);
        } else if (uvSize1 > 0 && uvSize2 > 0) {
            // YUV420P - Plane 1 (U) and Plane 2 (V)
            VkBufferImageCopy regionU{};
            regionU.bufferOffset = stagingAlloc.offset + ySize;
            regionU.imageSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
            regionU.imageSubresource.layerCount = 1;
            regionU.imageExtent = { m_width / 2, m_height / 2, 1 };
            vkCmdCopyBufferToImage(cmd, stagingAlloc.buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regionU);

            VkBufferImageCopy regionV{};
            regionV.bufferOffset = stagingAlloc.offset + ySize + uvSize1;
            regionV.imageSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_2_BIT;
            regionV.imageSubresource.layerCount = 1;
            regionV.imageExtent = { m_width / 2, m_height / 2, 1 };
            vkCmdCopyBufferToImage(cmd, stagingAlloc.buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regionV);
        }

        // Transition Transfer Dst -> Shader Read
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    }, true); // Sync for now
}

void VulkanTexture::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        spdlog::error("[VULKAN] Unsupported layout transition!");
        return;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

void VulkanTexture::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkCommandBuffer commandBuffer) {
    VkBufferImageCopy region{};
    region.bufferOffset = 0; // Using ring buffer this might be non-zero, but we inline it in loadFromFile now
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

} // namespace luma::render::vulkan
