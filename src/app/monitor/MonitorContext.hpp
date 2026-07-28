#pragma once

#include <memory>
#include <string>
#include <platform/common/IMonitor.hpp>
#include <platform/common/IPlatformBackend.hpp>
#include <render/vulkan/VulkanDevice.hpp>
#include <render/common/IRenderContext.hpp>
#include <render/common/IRenderSurface.hpp>
#include <render/common/RenderScheduler.hpp>
#include <render/common/RenderGraph.hpp>
#include <render/vulkan/upload/VulkanUploadManager.hpp>
#include <render/vulkan/memory/VulkanExternalMemoryManager.hpp>
#include <media/scheduler/MediaPlayer.hpp>
#include <platform/common/DisplayMode.hpp>
#include <render/vulkan/descriptor/DescriptorAllocator.hpp>
#include <render/vulkan/descriptor/DescriptorLayoutCache.hpp>

namespace luma::app::monitor {

class MonitorContext {
public:
    MonitorContext(
        std::shared_ptr<platform::IMonitor> monitor,
        platform::IPlatformBackend* platform,
        render::vulkan::VulkanDevice* vkDevice
    );
    ~MonitorContext();

    bool init();
    void start();
    void stop();

    void loadWallpaper(const std::string& path);
    void pause();
    void resume();

    std::shared_ptr<platform::IMonitor> getMonitor() const { return m_monitor; }

private:
    std::shared_ptr<platform::IMonitor> m_monitor;
    platform::IPlatformBackend* m_platform;
    render::vulkan::VulkanDevice* m_vkDevice;

    std::shared_ptr<platform::IWallpaperSurface> m_wallpaperSurface;
    std::shared_ptr<render::IRenderContext> m_renderContext;
    std::shared_ptr<render::IRenderSurface> m_renderSurface;

    std::shared_ptr<media::MediaPlayer> m_mediaPlayer;
    std::shared_ptr<render::vulkan::VulkanUploadManager> m_uploadManager;
    std::shared_ptr<render::vulkan::VulkanExternalMemoryManager> m_extMemManager;
    
    std::unique_ptr<render::RenderScheduler> m_scheduler;
    std::shared_ptr<render::RenderGraph> m_renderGraph;

    std::unique_ptr<render::vulkan::DescriptorAllocator> m_descriptorAllocator;
    std::unique_ptr<render::vulkan::DescriptorLayoutCache> m_descriptorCache;

    // Vulkan resources we need to hold onto for cleanup
    VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
    VkPipeline m_pipeline{VK_NULL_HANDLE};
};

} // namespace luma::app::monitor
