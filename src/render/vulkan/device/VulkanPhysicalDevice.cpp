#include "VulkanPhysicalDevice.hpp"
#include <core/Logging.hpp>
#include <vector>
#include <map>
#include <set>
#include <string>

namespace luma::render::vulkan {

VulkanPhysicalDevice::VulkanPhysicalDevice(VkInstance instance) : m_instance(instance) {}

bool VulkanPhysicalDevice::selectDevice(VkSurfaceKHR dummySurface) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        spdlog::error("[VULKAN] Failed to find GPUs with Vulkan support!");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    std::multimap<uint32_t, VkPhysicalDevice> candidates;

    for (const auto& device : devices) {
        uint32_t score = rateDeviceSuitability(device, dummySurface);
        candidates.insert(std::make_pair(score, device));
    }

    if (candidates.rbegin()->first > 0) {
        m_physicalDevice = candidates.rbegin()->second;
        m_queueFamilies = findQueueFamilies(m_physicalDevice, dummySurface);
        populateCapabilities(m_physicalDevice);
        spdlog::info("[VULKAN] Selected GPU: {}", m_capabilities.deviceName);
        return true;
    } else {
        spdlog::error("[VULKAN] Failed to find a suitable GPU!");
        return false;
    }
}

uint32_t VulkanPhysicalDevice::rateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR dummySurface) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // Require Vulkan 1.3
    if (deviceProperties.apiVersion < VK_API_VERSION_1_3) {
        return 0;
    }

    QueueFamilyIndices indices = findQueueFamilies(device, dummySurface);
    if (!indices.isComplete()) {
        return 0;
    }

    uint32_t score = 0;
    
    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Maximum possible size of textures affects our ability to render very large multi-monitor spanned wallpapers
    score += deviceProperties.limits.maxImageDimension2D;

    return score;
}

QueueFamilyIndices VulkanPhysicalDevice::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR dummySurface) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            // Dedicated transfer queue
            indices.transferFamily = i;
        }

        if (dummySurface != VK_NULL_HANDLE) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, dummySurface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }
        }

        i++;
    }
    
    // Fallback if no dedicated transfer queue
    if (!indices.transferFamily.has_value()) {
        indices.transferFamily = indices.graphicsFamily;
    }

    // Fallback if no dedicated present queue found via dummySurface
    if (!indices.presentFamily.has_value()) {
        indices.presentFamily = indices.graphicsFamily;
    }

    return indices;
}

void VulkanPhysicalDevice::populateCapabilities(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    m_capabilities.deviceName = properties.deviceName;
    m_capabilities.apiVersion = properties.apiVersion;
    m_capabilities.driverVersion = properties.driverVersion;
    m_capabilities.maxTextureDimension2D = properties.limits.maxImageDimension2D;
    m_capabilities.maxFramebufferWidth = properties.limits.maxFramebufferWidth;
    m_capabilities.maxFramebufferHeight = properties.limits.maxFramebufferHeight;

    // Check features (Vulkan 1.3 core features)
    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.pNext = &features13;

    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &features12;

    vkGetPhysicalDeviceFeatures2(device, &features2);

    m_capabilities.supportsDynamicRendering = features13.dynamicRendering == VK_TRUE;
    m_capabilities.supportsSynchronization2 = features13.synchronization2 == VK_TRUE;
    m_capabilities.supportsTimelineSemaphores = features12.timelineSemaphore == VK_TRUE;
    m_capabilities.supportsDescriptorIndexing = features12.descriptorIndexing == VK_TRUE;
    
    m_capabilities.hasDedicatedTransferQueue = (m_queueFamilies.transferFamily.value() != m_queueFamilies.graphicsFamily.value());

    // Check DMABUF extensions
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

    bool hasDmaBuf = false;
    bool hasFd = false;
    bool hasModifier = false;
    for (const auto& ext : extensions) {
        if (std::string(ext.extensionName) == VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME) hasDmaBuf = true;
        if (std::string(ext.extensionName) == VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME) hasFd = true;
        if (std::string(ext.extensionName) == VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME) hasModifier = true;
    }
    m_capabilities.supportsDmaBufImport = hasDmaBuf && hasFd && hasModifier;
}

} // namespace luma::render::vulkan
