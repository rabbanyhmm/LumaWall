#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-function"
#include <vk_mem_alloc.h> // VMA
#pragma GCC diagnostic pop
#include "VulkanPhysicalDevice.hpp"

namespace luma::render::vulkan {

class VulkanLogicalDevice {
public:
    VulkanLogicalDevice(VkInstance instance, std::shared_ptr<VulkanPhysicalDevice> physicalDevice);
    ~VulkanLogicalDevice();

    bool init();
    void cleanup();

    VkDevice getHandle() const { return m_device; }
    VkInstance getInstance() const { return m_instance; }
    std::shared_ptr<VulkanPhysicalDevice> getPhysicalDevice() const { return m_physicalDevice; }
    VmaAllocator getAllocator() const { return m_allocator; }

    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getPresentQueue() const { return m_presentQueue; }
    VkQueue getTransferQueue() const { return m_transferQueue; }

private:
    bool createDevice();
    bool createVmaAllocator();

    VkInstance m_instance{VK_NULL_HANDLE};
    std::shared_ptr<VulkanPhysicalDevice> m_physicalDevice;
    
    VkDevice m_device{VK_NULL_HANDLE};
    VmaAllocator m_allocator{VK_NULL_HANDLE};

    VkQueue m_graphicsQueue{VK_NULL_HANDLE};
    VkQueue m_presentQueue{VK_NULL_HANDLE};
    VkQueue m_transferQueue{VK_NULL_HANDLE};
};

} // namespace luma::render::vulkan
