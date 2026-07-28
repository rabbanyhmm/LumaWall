#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace luma::render::vulkan {

class VulkanInstance {
public:
    VulkanInstance();
    ~VulkanInstance();

    bool init(bool enableValidationLayers);
    void cleanup();

    VkInstance getHandle() const { return m_instance; }

private:
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void setupDebugMessenger();

    VkInstance m_instance{VK_NULL_HANDLE};
    VkDebugUtilsMessengerEXT m_debugMessenger{VK_NULL_HANDLE};
    bool m_enableValidationLayers{false};

    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
};

} // namespace luma::render::vulkan
