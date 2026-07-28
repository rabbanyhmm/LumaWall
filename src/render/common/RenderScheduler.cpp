#include "RenderScheduler.hpp"
#include <chrono>
#include <core/Logging.hpp>
#include "ITexture.hpp"
#include <core/error/ErrorTypes.hpp>

namespace luma::render {

RenderScheduler::RenderScheduler(std::shared_ptr<platform::IMonitor> monitor, INativeSurfaceProvider* provider, IRenderSurface* surface, IRenderContext* context)
    : m_monitor(std::move(monitor)), m_provider(provider), m_surface(surface), m_context(context) {
}

RenderScheduler::~RenderScheduler() {
    stop();
}

void RenderScheduler::setRenderGraph(std::shared_ptr<RenderGraph> graph) {
    m_graph = std::move(graph);
}

void RenderScheduler::start() {
    if (m_running) return;
    m_running = true;
    m_thread = std::thread(&RenderScheduler::loop, this);
}

void RenderScheduler::stop() {
    if (!m_running) return;
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void RenderScheduler::loop() {
    using namespace std::chrono;
    
    uint32_t targetFps = m_monitor ? m_monitor->getRefreshRate() : 60;
    if (targetFps == 0) targetFps = 60;
    
    auto frameDuration = duration<double>(1.0 / targetFps);
    (void)frameDuration;
    
    auto lastFrameTime = steady_clock::now();

    while (m_running) {
        if (m_provider) {
            m_provider->waitForNextFrame();
        }

        auto now = steady_clock::now();
        double deltaTime = duration<double>(now - lastFrameTime).count();
        lastFrameTime = now;

        if (m_graph && m_surface) {
            try {
                ITexture* target = m_surface->acquireNextFrame(m_context);
                if (target) {
                    RenderFrame frame{
                        .context = m_context,
                        .surface = m_surface,
                        .targetTexture = target,
                        .frameIndex = m_frameCount++,
                        .deltaTime = deltaTime
                    };
                    
                    m_graph->execute(frame);
                    m_surface->present(m_context);
                }
            } catch (const std::exception& e) {
                spdlog::critical("[RENDER] Exception in render loop: {}", e.what());
                core::error::ErrorHandler::instance().reportError(
                    core::error::ErrorCode::VulkanDeviceLost,
                    "GPU Device Lost or Unresponsive",
                    "Renderer"
                );
                m_running = false;
            }
        }
    }
}

} // namespace luma::render
