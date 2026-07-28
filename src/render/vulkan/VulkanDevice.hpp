#pragma once
#include <render/common/IRenderDevice.hpp>
#include <render/vulkan/device/VulkanLogicalDevice.hpp>
#include <memory>

namespace luma::render::vulkan {

class VulkanDevice : public IRenderDevice {
public:
    VulkanDevice(std::shared_ptr<VulkanLogicalDevice> logicalDevice);
    ~VulkanDevice() override;

    void waitIdle() override;

    std::unique_ptr<IRenderSurface> createSurface(INativeSurfaceProvider* surfaceProvider) override;
    std::unique_ptr<IRenderContext> createContext() override;

    std::shared_ptr<VulkanLogicalDevice> getLogicalDevice() const { return m_logicalDevice; }

private:
    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
};

} // namespace luma::render::vulkan
