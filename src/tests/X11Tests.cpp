#include <gtest/gtest.h>
#include <platform/x11/X11PlatformBackend.hpp>
#include <platform/PlatformFactory.hpp>
#include <core/events/EventBus.hpp>
#include <platform/common/IMonitor.hpp>
#include <cstdlib>

using namespace luma::platform;
using namespace luma::core::events;
using namespace luma::render;

class MockMonitor : public IMonitor {
public:
    MockMonitor(std::string id, uint32_t w, uint32_t h) : m_id(id), m_w(w), m_h(h) {}
    std::string getId() const override { return m_id; }
    std::string getName() const override { return "Mock"; }
    uint32_t getWidth() const override { return m_w; }
    uint32_t getHeight() const override { return m_h; }
    uint32_t getRefreshRate() const override { return 60; }
    float getScaleFactor() const override { return 1.0f; }
private:
    std::string m_id;
    uint32_t m_w, m_h;
};

class X11BackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        const char* display = std::getenv("DISPLAY");
        if (!display) {
            GTEST_SKIP() << "DISPLAY not set. Skipping X11 tests.";
        }
        eventBus = std::make_shared<EventBus>();
    }

    std::shared_ptr<EventBus> eventBus;
};

TEST_F(X11BackendTest, ConnectionLifecycleAndCapabilities) {
    x11::X11PlatformBackend backend;
    
    auto caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsWallpaperSurface);
    EXPECT_FALSE(caps.supportsLayerShell);

    ASSERT_TRUE(backend.init(eventBus));
}

TEST_F(X11BackendTest, WallpaperSurfaceCreation) {
    x11::X11PlatformBackend backend;
    ASSERT_TRUE(backend.init(eventBus));

    auto monitor = std::make_shared<MockMonitor>("0", 1920, 1080);
    auto surface = backend.createWallpaperSurface(monitor);
    
    ASSERT_NE(surface, nullptr);    auto info = surface->getSurfaceInfo();
    EXPECT_EQ(info.type, luma::render::NativeSurfaceType::X11);
    EXPECT_NE(info.window, nullptr);
    EXPECT_NE(info.display, nullptr);
    
    surface->show();
    surface->resize(1280, 720);
    surface->hide();
    
    surface->destroy();
}

TEST_F(X11BackendTest, FactorySelection) {
    unsetenv("WAYLAND_DISPLAY");
    
    auto backend = PlatformFactory::create();
    ASSERT_NE(backend, nullptr);
    
    auto caps = backend->getCapabilities();
    EXPECT_TRUE(caps.supportsWallpaperSurface);
    EXPECT_FALSE(caps.supportsLayerShell);
}
