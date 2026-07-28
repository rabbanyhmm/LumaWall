#include "RenderResourceManager.hpp"
#include "IRenderDevice.hpp"

namespace luma::render {

RenderResourceManager::RenderResourceManager(IRenderDevice* device) 
    : m_device(device) {
}

RenderResourceManager::~RenderResourceManager() {
    cleanup();
}

void RenderResourceManager::manageTexture(std::unique_ptr<ITexture> texture) {
    if (texture) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_textures.push_back(std::move(texture));
    }
}

void RenderResourceManager::manageBuffer(std::unique_ptr<IBuffer> buffer) {
    if (buffer) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_buffers.push_back(std::move(buffer));
    }
}

void RenderResourceManager::manageShader(std::unique_ptr<IShader> shader) {
    if (shader) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shaders.push_back(std::move(shader));
    }
}

void RenderResourceManager::cleanup() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // In Vulkan/OpenGL, we might need to wait for the device to be idle
    // before destroying resources.
    if (m_device) {
        m_device->waitIdle();
    }
    
    m_textures.clear();
    m_buffers.clear();
    m_shaders.clear();
}

} // namespace luma::render
