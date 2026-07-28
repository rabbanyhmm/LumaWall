#pragma once
#include <render/common/IRenderBackend.hpp>
#include <render/common/IRenderDevice.hpp>
#include <render/common/IRenderContext.hpp>
#include <render/common/IRenderSurface.hpp>
#include <render/common/ITexture.hpp>
#include <core/Logging.hpp>
#include <thread>
#include <chrono>

namespace luma::render::mock {

class MockTexture : public ITexture {
public:
    MockTexture(uint32_t w, uint32_t h) : width(w), height(h) {}
    uint32_t getWidth() const override { return width; }
    uint32_t getHeight() const override { return height; }
    TextureFormat getFormat() const override { return TextureFormat::RGBA8; }
private:
    uint32_t width, height;
};

class MockContext : public IRenderContext {
public:
    void begin(const RenderFrame& frame) override {}
    void end() override {}
    void submit(const RenderFrame& frame) override {}
    
    void bindShader(IShader*) override {}
    void bindTexture(uint32_t, ITexture*) override {}
    void bindVertexBuffer(IBuffer*) override {}
    void bindIndexBuffer(IBuffer*) override {}
    
    void draw(uint32_t, uint32_t) override {}
    void drawIndexed(uint32_t, uint32_t) override {}

    void beginRenderPass(ITexture*, float, float, float, float) override {}
    void endRenderPass() override {}
};

class MockSurface : public IRenderSurface {
public:
    bool build(uint32_t w, uint32_t h) override {
        width = w;
        height = h;
        texture = std::make_unique<MockTexture>(width, height);
        return true;
    }
    
    void destroy() override {
        texture.reset();
    }

    ITexture* acquireNextFrame(IRenderContext* context) override {
        return texture.get();
    }

    void present(IRenderContext* context) override {
        // Simulate VSync/Presentation time
        // spdlog::trace("[MockSurface] Presenting frame");
    }

    uint32_t getWidth() const override { return width; }
    uint32_t getHeight() const override { return height; }
    
private:
    uint32_t width{0}, height{0};
    std::unique_ptr<MockTexture> texture;
};

class MockDevice : public IRenderDevice {
public:
    void waitIdle() override {}

    std::unique_ptr<IRenderSurface> createSurface(INativeSurfaceProvider*) override {
        return std::make_unique<MockSurface>();
    }

    std::unique_ptr<IRenderContext> createContext() override {
        return std::make_unique<MockContext>();
    }
};

class MockBackend : public IRenderBackend {
public:
    bool init() override {
        spdlog::info("[MOCK RENDERER] Initialized");
        return true;
    }
    
    void shutdown() override {
        spdlog::info("[MOCK RENDERER] Shut down");
    }
    
    std::string getName() const override { return "Mock"; }
    
    std::unique_ptr<IRenderDevice> createDevice() override {
        return std::make_unique<MockDevice>();
    }
};

void registerMockBackend();

} // namespace luma::render::mock
