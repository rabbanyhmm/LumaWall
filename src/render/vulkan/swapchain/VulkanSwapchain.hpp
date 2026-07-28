#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <render/vulkan/device/VulkanLogicalDevice.hpp>
#include <platform/common/IWallpaperSurface.hpp>

namespace luma::render::vulkan {

class VulkanSwapchain {
public:
    VulkanSwapchain(std::shared_ptr<VulkanLogicalDevice> logicalDevice, VkSurfaceKHR surface);
    ~VulkanSwapchain();

    bool init(uint32_t width, uint32_t height, bool vsync = true);
    void cleanup();
    bool recreate(uint32_t width, uint32_t height, bool vsync = true);

    VkSwapchainKHR getHandle() const { return m_swapchain; }
    VkFormat getImageFormat() const { return m_imageFormat; }
    VkExtent2D getExtent() const { return m_extent; }
    const std::vector<VkImageView>& getImageViews() const { return m_imageViews; }
    const std::vector<VkImage>& getImages() const { return m_images; }
    uint32_t getImageCount() const { return static_cast<uint32_t>(m_images.size()); }

private:
    bool createSwapchain(uint32_t width, uint32_t height, bool vsync);
    bool createImageViews();
    
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool vsync);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
    VkSurfaceKHR m_surface{VK_NULL_HANDLE};
    
    VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;

    VkFormat m_imageFormat;
    VkExtent2D m_extent;
};

} // namespace luma::render::vulkan
