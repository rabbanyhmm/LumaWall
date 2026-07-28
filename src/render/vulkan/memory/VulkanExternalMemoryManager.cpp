#include "VulkanExternalMemoryManager.hpp"
#include <core/Logging.hpp>
#include <media/common/DmaBufFrame.hpp>
#include <render/vulkan/texture/VulkanTexture.hpp>
#include <unistd.h>
#include <vector>

namespace luma::render::vulkan {

VulkanExternalMemoryManager::VulkanExternalMemoryManager(std::shared_ptr<VulkanLogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice)) {
}

VulkanExternalMemoryManager::~VulkanExternalMemoryManager() {
}

bool VulkanExternalMemoryManager::isDmaBufSupported() const {
    return m_logicalDevice->getPhysicalDevice()->getCapabilities().supportsDmaBufImport;
}

uint32_t VulkanExternalMemoryManager::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_logicalDevice->getPhysicalDevice()->getHandle(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return ~0u;
}

bool VulkanExternalMemoryManager::importHardwareFrame(std::shared_ptr<media::Frame> frame, VkImage& outImage, class VulkanTexture* outTexture) {
    if (!frame || !frame->hwFrame || frame->hwFrame->getType() != media::HardwareFrameType::DmaBuf) {
        return false;
    }

    if (!isDmaBufSupported()) {
        spdlog::warn("[VULKAN] DMABUF import requested but not supported by device");
        return false;
    }

    auto dmaFrame = static_cast<media::DmaBufFrame*>(frame->hwFrame->getNativeHandle());
    if (!dmaFrame || dmaFrame->getPlanes().empty()) return false;

    VkDevice device = m_logicalDevice->getHandle();
    VkFormat format = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
    uint32_t width = frame->width;
    uint32_t height = frame->height;

    // 1. Setup DRM format modifiers
    std::vector<VkSubresourceLayout> planeLayouts(dmaFrame->getPlanes().size());
    for (size_t i = 0; i < dmaFrame->getPlanes().size(); ++i) {
        planeLayouts[i].offset = dmaFrame->getPlanes()[i].offset;
        planeLayouts[i].rowPitch = dmaFrame->getPlanes()[i].pitch;
        planeLayouts[i].size = 0; // Usually driver infers this
        planeLayouts[i].arrayPitch = 0;
        planeLayouts[i].depthPitch = 0;
    }

    VkImageDrmFormatModifierExplicitCreateInfoEXT modifierInfo{};
    modifierInfo.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT;
    modifierInfo.drmFormatModifier = dmaFrame->getFormatModifier();
    modifierInfo.drmFormatModifierPlaneCount = static_cast<uint32_t>(planeLayouts.size());
    modifierInfo.pPlaneLayouts = planeLayouts.data();

    // 2. Setup external memory handle type
    VkExternalMemoryImageCreateInfo extImageInfo{};
    extImageInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
    extImageInfo.pNext = &modifierInfo;
    extImageInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

    // 3. Create VkImage
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext = &extImageInfo;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = { width, height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device, &imageInfo, nullptr, &outImage) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create DMABUF VkImage");
        return false;
    }

    // 4. Get memory requirements
    VkImageMemoryRequirementsInfo2 reqInfo{};
    reqInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
    reqInfo.image = outImage;

    VkMemoryRequirements2 reqs2{};
    reqs2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
    vkGetImageMemoryRequirements2(device, &reqInfo, &reqs2);

    // 5. Import DMABUF memory
    VkImportMemoryFdInfoKHR importInfo{};
    importInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
    importInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    importInfo.fd = dup(dmaFrame->getPlanes()[0].fd); // Duplicate because Vulkan takes ownership

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = &importInfo;
    allocInfo.allocationSize = reqs2.memoryRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(reqs2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkDeviceMemory importedMemory = VK_NULL_HANDLE;
    if (vkAllocateMemory(device, &allocInfo, nullptr, &importedMemory) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to allocate memory for DMABUF");
        vkDestroyImage(device, outImage, nullptr);
        close(importInfo.fd); // Close duplicated FD on failure
        return false;
    }

    // 6. Bind memory
    VkBindImageMemoryInfo bindInfo{};
    bindInfo.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
    bindInfo.image = outImage;
    bindInfo.memory = importedMemory;
    bindInfo.memoryOffset = 0;

    if (vkBindImageMemory2(device, 1, &bindInfo) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to bind DMABUF memory");
        vkFreeMemory(device, importedMemory, nullptr);
        vkDestroyImage(device, outImage, nullptr);
        return false;
    }

    // 7. Create YCbCr Conversion
    VkSamplerYcbcrConversion ycbcrConversion = VK_NULL_HANDLE;
    VkSamplerYcbcrConversionCreateInfo ycbcrInfo{};
    ycbcrInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO;
    ycbcrInfo.format = format;
    ycbcrInfo.ycbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709;
    ycbcrInfo.ycbcrRange = VK_SAMPLER_YCBCR_RANGE_ITU_NARROW;
    ycbcrInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    ycbcrInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    ycbcrInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    ycbcrInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    ycbcrInfo.xChromaOffset = VK_CHROMA_LOCATION_MIDPOINT;
    ycbcrInfo.yChromaOffset = VK_CHROMA_LOCATION_MIDPOINT;
    ycbcrInfo.chromaFilter = VK_FILTER_LINEAR;
    ycbcrInfo.forceExplicitReconstruction = VK_FALSE;

    if (vkCreateSamplerYcbcrConversion(device, &ycbcrInfo, nullptr, &ycbcrConversion) != VK_SUCCESS) {
        vkFreeMemory(device, importedMemory, nullptr);
        vkDestroyImage(device, outImage, nullptr);
        return false;
    }

    // 8. Create ImageView
    VkImageView imageView = VK_NULL_HANDLE;
    VkSamplerYcbcrConversionInfo samplerYcbcrInfo{};
    samplerYcbcrInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO;
    samplerYcbcrInfo.conversion = ycbcrConversion;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = &samplerYcbcrInfo;
    viewInfo.image = outImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        vkDestroySamplerYcbcrConversion(device, ycbcrConversion, nullptr);
        vkFreeMemory(device, importedMemory, nullptr);
        vkDestroyImage(device, outImage, nullptr);
        return false;
    }

    // 9. Create Sampler
    VkSampler sampler = VK_NULL_HANDLE;
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

    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        vkDestroyImageView(device, imageView, nullptr);
        vkDestroySamplerYcbcrConversion(device, ycbcrConversion, nullptr);
        vkFreeMemory(device, importedMemory, nullptr);
        vkDestroyImage(device, outImage, nullptr);
        return false;
    }

    // 10. Pass to VulkanTexture
    outTexture->createFromExternalMemory(outImage, importedMemory, imageView, ycbcrConversion, sampler, width, height);
    return true;
}

} // namespace luma::render::vulkan
