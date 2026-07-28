#pragma once

namespace luma::render {

class ITexture;
class IBuffer;
class IShader;
struct RenderFrame;

class IRenderContext {
public:
    virtual ~IRenderContext() = default;

    virtual void begin(const RenderFrame& frame) = 0;
    virtual void end() = 0;
    
    virtual void submit(const RenderFrame& frame) = 0;

    // Drawing commands
    virtual void bindShader(IShader* shader) = 0;
    virtual void bindTexture(uint32_t slot, ITexture* texture) = 0;
    virtual void bindVertexBuffer(IBuffer* buffer) = 0;
    virtual void bindIndexBuffer(IBuffer* buffer) = 0;
    
    virtual void draw(uint32_t vertexCount, uint32_t instanceCount) = 0;
    virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount) = 0;

    // Render pass semantics
    virtual void beginRenderPass(ITexture* renderTarget, float r=0.f, float g=0.f, float b=0.f, float a=1.f) = 0;
    virtual void endRenderPass() = 0;
};

} // namespace luma::render
