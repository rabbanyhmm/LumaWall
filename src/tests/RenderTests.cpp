#include <gtest/gtest.h>
#include <render/common/BackendRegistry.hpp>
#include <render/common/IRenderBackend.hpp>
#include <render/common/IRenderDevice.hpp>
#include <render/common/IRenderContext.hpp>
#include <render/common/IRenderSurface.hpp>
#include <render/common/RenderScheduler.hpp>
#include <render/common/RenderGraph.hpp>
#include <render/common/RenderResourceManager.hpp>
#include <render/mock/MockRenderer.hpp>
#include <platform/common/IMonitor.hpp>
#include <thread>

using namespace luma::render;
using namespace luma::platform;

class DummyMonitor : public IMonitor {
public:
    std::string getId() const override { return "0"; }
    std::string getName() const override { return "Dummy"; }
    uint32_t getWidth() const override { return 1920; }
    uint32_t getHeight() const override { return 1080; }
    uint32_t getRefreshRate() const override { return 144; }
    float getScaleFactor() const override { return 1.0f; }
};

class DummySurfaceProvider : public INativeSurfaceProvider {
public:
    NativeSurfaceInfo getSurfaceInfo() const override {
        return NativeSurfaceInfo{NativeSurfaceType::Mock, nullptr, nullptr};
    }
    
    void waitForNextFrame() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(7)); // Simulate 144Hz
    }
};

TEST(RenderTests, BackendRegistrySelection) {
    luma::render::mock::registerMockBackend();
    
    auto& registry = BackendRegistry::get();
    auto backends = registry.getAvailableBackends();
    ASSERT_FALSE(backends.empty());
    
    // We expect at least the Mock backend to be registered
    bool hasMock = false;
    for (const auto& name : backends) {
        if (name == "Mock") hasMock = true;
    }
    EXPECT_TRUE(hasMock);

    auto backend = registry.createBestAvailableBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_EQ(backend->getName(), "Mock");
    
    ASSERT_TRUE(backend->init());
    backend->shutdown();
}

TEST(RenderTests, SchedulerAndGraphExecution) {
    luma::render::mock::registerMockBackend();

    auto backend = BackendRegistry::get().createBestAvailableBackend();
    ASSERT_NE(backend, nullptr);
    backend->init();

    auto device = backend->createDevice();
    auto context = device->createContext();
    auto monitor = std::make_shared<DummyMonitor>();
    DummySurfaceProvider provider;
    
    auto surface = device->createSurface(&provider);
    surface->build(1920, 1080);

    RenderScheduler scheduler(monitor, &provider, surface.get(), context.get());
    
    auto graph = std::make_shared<RenderGraph>();
    
    class TestNode : public RenderGraphNode {
    public:
        int& counter;
        TestNode(int& c) : counter(c) {}
        void execute(const RenderFrame& /*frame*/) override {
            counter++;
        }
    };
    
    int executionCount = 0;
    graph->addNode(std::make_unique<TestNode>(executionCount));
    
    scheduler.setRenderGraph(graph);
    
    scheduler.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    scheduler.stop();
    
    // At 144Hz, 50ms should yield roughly 7 frames (50 / (1000/144) = 7.2)
    EXPECT_GT(executionCount, 0);
    EXPECT_LT(executionCount, 15);
}

TEST(RenderTests, ResourceManager) {
    luma::render::mock::registerMockBackend();

    auto backend = BackendRegistry::get().createBestAvailableBackend();
    backend->init();
    auto device = backend->createDevice();
    
    RenderResourceManager manager(device.get());
    // Since we don't have concrete implementations for Textures/Buffers in Mock yet,
    // we just test the instantiation and destruction safety.
    manager.cleanup();
}
