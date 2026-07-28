#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <media/common/IHardwareFrame.hpp>
#include <render/vulkan/device/VulkanLogicalDevice.hpp>

namespace luma::media {
    struct Frame;
}

namespace luma::render::vulkan {

class VulkanExternalMemoryManager {
public:
    VulkanExternalMemoryManager(std::shared_ptr<VulkanLogicalDevice> logicalDevice);
    ~VulkanExternalMemoryManager();

    // Validates if the physical device supports DMABUF import
    bool isDmaBufSupported() const;

    // Placeholder for future DMABUF import logic
    bool importHardwareFrame(std::shared_ptr<media::Frame> frame, VkImage& outImage, class VulkanTexture* outTexture);

private:
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
};

} // namespace luma::render::vulkan
