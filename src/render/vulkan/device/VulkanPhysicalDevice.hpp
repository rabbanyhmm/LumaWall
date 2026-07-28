#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include "GpuCapabilities.hpp"

namespace luma::render::vulkan {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;

    bool isComplete() const {
        return graphicsFamily.has_value(); // Present will depend on surface later
    }
};

class VulkanPhysicalDevice {
public:
    VulkanPhysicalDevice(VkInstance instance);

    bool selectDevice(VkSurfaceKHR dummySurface = VK_NULL_HANDLE);
    
    VkPhysicalDevice getHandle() const { return m_physicalDevice; }
    const GpuCapabilities& getCapabilities() const { return m_capabilities; }
    QueueFamilyIndices getQueueFamilies() const { return m_queueFamilies; }

private:
    uint32_t rateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR dummySurface);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR dummySurface);
    void populateCapabilities(VkPhysicalDevice device);

    VkInstance m_instance{VK_NULL_HANDLE};
    VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
    GpuCapabilities m_capabilities;
    QueueFamilyIndices m_queueFamilies;
};

} // namespace luma::render::vulkan
