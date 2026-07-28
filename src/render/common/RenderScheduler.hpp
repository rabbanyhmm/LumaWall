#pragma once
#include <memory>
#include <atomic>
#include <thread>
#include "IRenderSurface.hpp"
#include "IRenderContext.hpp"
#include "RenderGraph.hpp"
#include <platform/common/IMonitor.hpp>

#include "INativeSurfaceProvider.hpp"

namespace luma::render {

class RenderScheduler {
public:
    RenderScheduler(std::shared_ptr<platform::IMonitor> monitor, INativeSurfaceProvider* provider, IRenderSurface* surface, IRenderContext* context);
    ~RenderScheduler();

    void setRenderGraph(std::shared_ptr<RenderGraph> graph);

    void start();
    void stop();

    bool isRunning() const { return m_running; }

private:
    void loop();

    std::shared_ptr<platform::IMonitor> m_monitor;
    INativeSurfaceProvider* m_provider;
    IRenderSurface* m_surface;
    IRenderContext* m_context;
    std::shared_ptr<RenderGraph> m_graph;
    
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    
    uint64_t m_frameCount{0};
};

} // namespace luma::render
