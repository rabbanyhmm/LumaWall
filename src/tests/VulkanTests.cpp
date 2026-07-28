#include <gtest/gtest.h>
#include <render/vulkan/VulkanBackend.hpp>
#include <render/common/BackendRegistry.hpp>
#include <render/common/IRenderDevice.hpp>

using namespace luma::render;
using namespace luma::render::vulkan;

TEST(VulkanTests, BackendInitialization) {
    registerVulkanBackend();

    auto& registry = BackendRegistry::get();
    auto backends = registry.getAvailableBackends();
    ASSERT_FALSE(backends.empty());

    bool hasVulkan = false;
    for (const auto& name : backends) {
        if (name == "Vulkan 1.3") hasVulkan = true;
    }
    
    if (!hasVulkan) {
        GTEST_SKIP() << "Vulkan 1.3 backend not supported on this system";
    }

    auto backend = registry.createBestAvailableBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_EQ(backend->getName(), "Vulkan 1.3");

    // Init tests instance, physical device, logical device, and VMA creation
    bool initSuccess = backend->init();
    EXPECT_TRUE(initSuccess);
    
    if (initSuccess) {
        auto device = backend->createDevice();
        ASSERT_NE(device, nullptr);
        
        device->waitIdle();
        device.reset();
        backend->shutdown();
    }
}
