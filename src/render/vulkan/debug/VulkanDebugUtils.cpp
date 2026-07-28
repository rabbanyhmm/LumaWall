#include "VulkanDebugUtils.hpp"
#include <core/Logging.hpp>

namespace luma::render::vulkan {

void VulkanDebugUtils::setObjectName(VkDevice device, uint64_t objectHandle, VkObjectType objectType, const std::string& name) {
#ifndef NDEBUG
    if (device == VK_NULL_HANDLE || objectHandle == 0) return;

    auto func = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
    if (func != nullptr) {
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = objectType;
        nameInfo.objectHandle = objectHandle;
        nameInfo.pObjectName = name.c_str();

        func(device, &nameInfo);
    }
#else
    (void)device;
    (void)objectHandle;
    (void)objectType;
    (void)name;
#endif
}

} // namespace luma::render::vulkan
