#include "VulkanDevice.hpp"
#include <core/Logging.hpp>
#include <render/common/IRenderSurface.hpp>
#include <render/common/IRenderContext.hpp>
#include <render/vulkan/swapchain/VulkanSurface.hpp>
#include <render/vulkan/command/VulkanContext.hpp>
#include <render/vulkan/command/CommandPool.hpp>

namespace luma::render::vulkan {

VulkanDevice::VulkanDevice(std::shared_ptr<VulkanLogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice)) {
}

VulkanDevice::~VulkanDevice() {
    waitIdle();
}

void VulkanDevice::waitIdle() {
    if (m_logicalDevice && m_logicalDevice->getHandle() != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_logicalDevice->getHandle());
    }
}

std::unique_ptr<IRenderSurface> VulkanDevice::createSurface(INativeSurfaceProvider* surfaceProvider) {
    if (!surfaceProvider) return nullptr;
    
    auto surface = std::make_unique<VulkanSurface>(m_logicalDevice, m_logicalDevice->getInstance(), surfaceProvider->getSurfaceInfo());
    return surface;
}

std::unique_ptr<IRenderContext> VulkanDevice::createContext() {
    auto commandPool = std::make_shared<CommandPool>(m_logicalDevice, m_logicalDevice->getPhysicalDevice()->getQueueFamilies().graphicsFamily.value());
    commandPool->init();
    auto context = std::make_unique<VulkanContext>(m_logicalDevice, commandPool);
    if (!context->init(2)) { // Max frames in flight = 2
        spdlog::error("[VULKAN] Failed to initialize VulkanContext");
        return nullptr;
    }
    return context;
}

} // namespace luma::render::vulkan
