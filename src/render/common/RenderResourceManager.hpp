#pragma once
#include <memory>
#include <vector>
#include <mutex>
#include "ITexture.hpp"
#include "IBuffer.hpp"
#include "IShader.hpp"

namespace luma::render {

class IRenderDevice;

class RenderResourceManager {
public:
    RenderResourceManager(IRenderDevice* device);
    ~RenderResourceManager();

    // Factory methods could go here in a real implementation
    // For now we just manage lifetimes
    
    void manageTexture(std::unique_ptr<ITexture> texture);
    void manageBuffer(std::unique_ptr<IBuffer> buffer);
    void manageShader(std::unique_ptr<IShader> shader);

    void cleanup();

private:
    IRenderDevice* m_device;
    
    std::mutex m_mutex;
    std::vector<std::unique_ptr<ITexture>> m_textures;
    std::vector<std::unique_ptr<IBuffer>> m_buffers;
    std::vector<std::unique_ptr<IShader>> m_shaders;
};

} // namespace luma::render
