#pragma once
#include <render/common/IRenderBackend.hpp>
#include <render/vulkan/instance/VulkanInstance.hpp>
#include <render/vulkan/device/VulkanPhysicalDevice.hpp>
#include <render/vulkan/device/VulkanLogicalDevice.hpp>
#include <memory>

namespace luma::render::vulkan {

class VulkanBackend : public IRenderBackend {
public:
    VulkanBackend();
    ~VulkanBackend() override;

    bool init() override;
    void shutdown() override;
    
    std::string getName() const override { return "Vulkan 1.3"; }
    
    std::unique_ptr<IRenderDevice> createDevice() override;

private:
    std::unique_ptr<VulkanInstance> m_instance;
    std::shared_ptr<VulkanPhysicalDevice> m_physicalDevice;
    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
};

void registerVulkanBackend();

} // namespace luma::render::vulkan
