#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <memory>

namespace luma::render::vulkan {

class VulkanLogicalDevice; // Forward declaration

class VulkanDebugUtils {
public:
    static void setObjectName(VkDevice device, uint64_t objectHandle, VkObjectType objectType, const std::string& name);
    
    template<typename T>
    static void setObjectName(VkDevice device, T objectHandle, VkObjectType objectType, const std::string& name) {
        setObjectName(device, reinterpret_cast<uint64_t>(objectHandle), objectType, name);
    }
};

} // namespace luma::render::vulkan
