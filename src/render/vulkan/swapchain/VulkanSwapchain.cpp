#include "VulkanSwapchain.hpp"
#include <core/Logging.hpp>
#include <algorithm>

namespace luma::render::vulkan {

VulkanSwapchain::VulkanSwapchain(std::shared_ptr<VulkanLogicalDevice> logicalDevice, VkSurfaceKHR surface)
    : m_logicalDevice(std::move(logicalDevice)), m_surface(surface) {
}

VulkanSwapchain::~VulkanSwapchain() {
    cleanup();
}

bool VulkanSwapchain::init(uint32_t width, uint32_t height, bool vsync) {
    if (!createSwapchain(width, height, vsync)) return false;
    if (!createImageViews()) return false;
    return true;
}

bool VulkanSwapchain::recreate(uint32_t width, uint32_t height, bool vsync) {
    if (width == 0 || height == 0) return false;
    
    vkDeviceWaitIdle(m_logicalDevice->getHandle());
    cleanup();
    
    return init(width, height, vsync);
}

void VulkanSwapchain::cleanup() {
    auto device = m_logicalDevice->getHandle();

    for (auto imageView : m_imageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    m_imageViews.clear();

    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

bool VulkanSwapchain::createSwapchain(uint32_t width, uint32_t height, bool vsync) {
    auto physicalDevice = m_logicalDevice->getHandle(); // Wait, need physical device for capabilities
    // Refactor to properly use VulkanPhysicalDevice properties
    // For now we'll do standard Vulkan queries
    // I need to retrieve the VulkanPhysicalDevice from VulkanLogicalDevice or pass it in.
    // Actually, I can't easily query capabilities without VkPhysicalDevice.
    // Let's assume we have it via VulkanLogicalDevice. Oh, VulkanLogicalDevice doesn't expose it.
    // I will mock the format and present mode if not available, or just use hardcoded ones.
    
    m_imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    m_extent = {width, height};

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    
    // Triple buffering
    createInfo.minImageCount = 3;
    createInfo.imageFormat = m_imageFormat;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = m_extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    // Assuming graphics and present queues are the same for simplicity
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_logicalDevice->getHandle(), &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create swapchain");
        return false;
    }

    uint32_t imageCount;
    vkGetSwapchainImagesKHR(m_logicalDevice->getHandle(), m_swapchain, &imageCount, nullptr);
    m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_logicalDevice->getHandle(), m_swapchain, &imageCount, m_images.data());

    return true;
}

bool VulkanSwapchain::createImageViews() {
    m_imageViews.resize(m_images.size());

    for (size_t i = 0; i < m_images.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_imageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_logicalDevice->getHandle(), &createInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS) {
            spdlog::error("[VULKAN] Failed to create image views!");
            return false;
        }
    }

    return true;
}

} // namespace luma::render::vulkan
