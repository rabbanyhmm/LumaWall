#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-function"
#define VMA_IMPLEMENTATION
#include "VulkanLogicalDevice.hpp"
#pragma GCC diagnostic pop

#include <core/Logging.hpp>
#include <set>
#include <vector>

namespace luma::render::vulkan {

VulkanLogicalDevice::VulkanLogicalDevice(VkInstance instance, std::shared_ptr<VulkanPhysicalDevice> physicalDevice)
    : m_instance(instance), m_physicalDevice(std::move(physicalDevice)) {
}

VulkanLogicalDevice::~VulkanLogicalDevice() {
    cleanup();
}

bool VulkanLogicalDevice::init() {
    if (!createDevice()) return false;
    if (!createVmaAllocator()) return false;
    return true;
}

void VulkanLogicalDevice::cleanup() {
    if (m_allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(m_allocator);
        m_allocator = VK_NULL_HANDLE;
    }
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }
}

bool VulkanLogicalDevice::createDevice() {
    auto indices = m_physicalDevice->getQueueFamilies();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.transferFamily.value()
    };
    
    if (indices.presentFamily.has_value()) {
        uniqueQueueFamilies.insert(indices.presentFamily.value());
    }

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.timelineSemaphore = VK_TRUE;
    features12.descriptorIndexing = VK_TRUE;
    features12.bufferDeviceAddress = VK_TRUE;
    features12.pNext = &features13;

    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &features12;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &features2;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    // Device extensions (e.g., VK_KHR_swapchain)
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    if (m_physicalDevice->getCapabilities().supportsDmaBufImport) {
        deviceExtensions.push_back(VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
        deviceExtensions.push_back(VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME);
    }
    
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(m_physicalDevice->getHandle(), &createInfo, nullptr, &m_device) != VK_SUCCESS) {
        spdlog::error("[VULKAN] Failed to create logical device!");
        return false;
    }

    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.transferFamily.value(), 0, &m_transferQueue);
    
    if (indices.presentFamily.has_value()) {
        vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
    }

    spdlog::info("[VULKAN] Logical Device and Queues initialized");
    return true;
}

bool VulkanLogicalDevice::createVmaAllocator() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_physicalDevice->getHandle();
    allocatorInfo.device = m_device;
    allocatorInfo.instance = m_instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // Required for sync2 / Vulkan 1.3 buffers often

    if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) {
        spdlog::error("[VMA] Failed to create VMA Allocator");
        return false;
    }

    spdlog::info("[VMA] Vulkan Memory Allocator initialized successfully");
    return true;
}

} // namespace luma::render::vulkan
