#pragma once
#include <render/common/IRenderSurface.hpp>
#include <render/common/INativeSurfaceProvider.hpp>
#include <render/vulkan/swapchain/VulkanSwapchain.hpp>
#include <memory>

namespace luma::render::vulkan {

class VulkanSurface : public IRenderSurface {
public:
    VulkanSurface(std::shared_ptr<VulkanLogicalDevice> logicalDevice, VkInstance instance, const NativeSurfaceInfo& info);
    ~VulkanSurface() override;

    bool build(uint32_t width, uint32_t height) override;
    void destroy() override;

    ITexture* acquireNextFrame(IRenderContext* context) override;
    void present(IRenderContext* context) override;

    uint32_t getWidth() const override;
    uint32_t getHeight() const override;

private:
    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
    VkInstance m_instance{VK_NULL_HANDLE};
    VkSurfaceKHR m_surface{VK_NULL_HANDLE};
    std::unique_ptr<VulkanSwapchain> m_swapchain;
    std::vector<std::unique_ptr<ITexture>> m_swapchainTextures;
    uint32_t m_currentImageIndex = 0;
};

} // namespace luma::render::vulkan
