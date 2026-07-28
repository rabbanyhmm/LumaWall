#define VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <wayland-client.h>
#include <xcb/xcb.h>
#include <vulkan/vulkan.h>
#include "VulkanSurface.hpp"
#include <core/Logging.hpp>
#include <render/vulkan/instance/VulkanInstance.hpp>
#include <render/vulkan/command/VulkanContext.hpp>
#include <render/vulkan/texture/VulkanTexture.hpp>

namespace luma::render::vulkan {

VulkanSurface::VulkanSurface(std::shared_ptr<VulkanLogicalDevice> logicalDevice, VkInstance instance, const NativeSurfaceInfo& info)
    : m_logicalDevice(std::move(logicalDevice)), m_instance(instance) {
    
    if (info.type == NativeSurfaceType::X11) {
        VkXcbSurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        createInfo.connection = reinterpret_cast<xcb_connection_t*>(info.display);
        createInfo.window = static_cast<xcb_window_t>(reinterpret_cast<uintptr_t>(info.window));
        
        if (vkCreateXcbSurfaceKHR(m_instance, &createInfo, nullptr, &m_surface) != VK_SUCCESS) {
            spdlog::error("[VULKAN] Failed to create XCB surface");
        }
    } else if (info.type == NativeSurfaceType::Wayland) {
        VkWaylandSurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        createInfo.display = reinterpret_cast<wl_display*>(info.display);
        createInfo.surface = reinterpret_cast<wl_surface*>(info.window);
        
        if (vkCreateWaylandSurfaceKHR(m_instance, &createInfo, nullptr, &m_surface) != VK_SUCCESS) {
            spdlog::error("[VULKAN] Failed to create Wayland surface");
        }
    } else {
        spdlog::warn("[VULKAN] Mock surface or unsupported surface type provided");
        m_surface = VK_NULL_HANDLE;
    }
}

VulkanSurface::~VulkanSurface() {
    m_swapchain.reset();
    if (m_surface != VK_NULL_HANDLE && m_instance != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    }
}

bool VulkanSurface::build(uint32_t width, uint32_t height) {
    m_swapchain = std::make_unique<VulkanSwapchain>(m_logicalDevice, m_surface);
    bool result = m_swapchain->init(width, height);
    if (!result) return false;

    m_swapchainTextures.clear();
    auto format = m_swapchain->getImageFormat();
    
    // Create VulkanTexture objects to wrap swapchain images
    const auto& images = m_swapchain->getImages();
    const auto& imageViews = m_swapchain->getImageViews();
    for (size_t i = 0; i < images.size(); i++) {
        auto tex = std::make_unique<VulkanTexture>(m_logicalDevice);
        tex->createFromImage(images[i], imageViews[i], format, width, height);
        m_swapchainTextures.push_back(std::move(tex));
    }
    
    return true;
}

void VulkanSurface::destroy() {
    m_swapchain.reset();
}

ITexture* VulkanSurface::acquireNextFrame(IRenderContext* context) {
    if (!m_swapchain) return nullptr;
    
    auto vkContext = dynamic_cast<VulkanContext*>(context);
    if (!vkContext) return nullptr;
    
    auto frameContext = vkContext->getCurrentFrameContext();
    if (!frameContext) return nullptr;
    
    frameContext->waitForFence();
    
    VkResult result = vkAcquireNextImageKHR(
        m_logicalDevice->getHandle(), 
        m_swapchain->getHandle(), 
        UINT64_MAX, 
        frameContext->getImageAvailableSemaphore(), 
        VK_NULL_HANDLE, 
        &m_currentImageIndex
    );
    
    if (result == VK_ERROR_DEVICE_LOST) {
        throw std::runtime_error("VK_ERROR_DEVICE_LOST");
    } else if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Swapchain outdated — signal caller to recreate
        return nullptr;
    } else if (result == VK_ERROR_SURFACE_LOST_KHR) {
        // Surface was lost (e.g. X11 window destroyed). Log once and bail.
        spdlog::error("[VULKAN] Surface lost (VK_ERROR_SURFACE_LOST_KHR) — window may have been destroyed");
        return nullptr;
    } else if (result != VK_SUCCESS) {
        // Rate-limit generic errors to avoid journal spam
        static uint32_t s_failCount = 0;
        s_failCount++;
        if (s_failCount == 1 || s_failCount % 100 == 0) {
            spdlog::error("[VULKAN] Failed to acquire next image: VkResult={} (occurrence #{})", static_cast<int>(result), s_failCount);
        }
        return nullptr;
    }
    
    if (m_currentImageIndex < m_swapchainTextures.size()) {
        return m_swapchainTextures[m_currentImageIndex].get();
    }
    
    return nullptr;
}

void VulkanSurface::present(IRenderContext* context) {
    if (!m_swapchain) return;

    auto vkContext = dynamic_cast<VulkanContext*>(context);
    if (!vkContext) return;
    
    auto frameContext = vkContext->getCurrentFrameContext();
    
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    
    VkSemaphore signalSemaphores[] = {frameContext->getRenderFinishedSemaphore()};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapchains[] = {m_swapchain->getHandle()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &m_currentImageIndex;
    
    VkResult result = vkQueuePresentKHR(m_logicalDevice->getPresentQueue(), &presentInfo);
    
    if (result == VK_ERROR_DEVICE_LOST) {
        throw std::runtime_error("VK_ERROR_DEVICE_LOST");
    } else if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Handled in next acquire
    }
    
    vkContext->advanceFrame();
}

uint32_t VulkanSurface::getWidth() const {
    return m_swapchain ? m_swapchain->getExtent().width : 0;
}

uint32_t VulkanSurface::getHeight() const {
    return m_swapchain ? m_swapchain->getExtent().height : 0;
}

} // namespace luma::render::vulkan
