#include "VulkanBackend.hpp"
#include "VulkanDevice.hpp"
#include <core/Logging.hpp>
#include <render/common/BackendRegistry.hpp>
#include <render/common/IRenderBackendFactory.hpp>

namespace luma::render::vulkan {

VulkanBackend::VulkanBackend() = default;

VulkanBackend::~VulkanBackend() {
    shutdown();
}

bool VulkanBackend::init() {
    m_instance = std::make_unique<VulkanInstance>();
    
    // In production, we enable validation layers only in debug builds
#ifdef NDEBUG
    bool enableValidationLayers = false;
#else
    bool enableValidationLayers = true;
#endif

    if (!m_instance->init(enableValidationLayers)) {
        return false;
    }

    m_physicalDevice = std::make_shared<VulkanPhysicalDevice>(m_instance->getHandle());
    if (!m_physicalDevice->selectDevice()) {
        return false;
    }

    m_logicalDevice = std::make_shared<VulkanLogicalDevice>(m_instance->getHandle(), m_physicalDevice);
    if (!m_logicalDevice->init()) {
        return false;
    }

    spdlog::info("[VULKAN] Backend initialized successfully");
    return true;
}

void VulkanBackend::shutdown() {
    m_logicalDevice.reset();
    m_physicalDevice.reset();
    m_instance.reset();
}

std::unique_ptr<IRenderDevice> VulkanBackend::createDevice() {
    return std::make_unique<VulkanDevice>(m_logicalDevice);
}

// ----------------------------------------------------
// Factory and Registration
// ----------------------------------------------------
class VulkanBackendFactory : public IRenderBackendFactory {
public:
    std::string getName() const override { return "Vulkan 1.3"; }
    
    int getPriority() const override { return 100; } // Highest priority
    
    bool isSupported() const override {
        // Technically we should check if vulkan library loads here, 
        // but for now we assume it's linked
        return true; 
    }

    std::unique_ptr<IRenderBackend> create() override {
        return std::make_unique<VulkanBackend>();
    }
};

void registerVulkanBackend() {
    static bool registered = false;
    if (!registered) {
        BackendRegistry::get().registerFactory(std::make_unique<VulkanBackendFactory>());
        registered = true;
    }
}

} // namespace luma::render::vulkan
