#include "MonitorContext.hpp"
#include <core/Logging.hpp>
#include <core/SettingsManager.hpp>
#include <core/system/CapabilityManager.hpp>
#include <render/vulkan/shader/ShaderLoader.hpp>
#include <render/vulkan/pipeline/PipelineBuilder.hpp>
#include <render/vulkan/descriptor/DescriptorAllocator.hpp>
#include <render/vulkan/descriptor/DescriptorLayoutCache.hpp>
#include <render/vulkan/descriptor/DescriptorBuilder.hpp>
#include <render/vulkan/texture/VulkanTexture.hpp>
#include <render/vulkan/texture/VulkanTextureUploader.hpp>
#include <render/vulkan/FirstRenderNode.hpp>
#include <render/vulkan/pipeline/VulkanPipelineCache.hpp>
#include <media/common/FileMediaSource.hpp>
#include <thread>
#include <chrono>

namespace luma::app::monitor {

MonitorContext::MonitorContext(
    std::shared_ptr<platform::IMonitor> monitor,
    platform::IPlatformBackend* platform,
    render::vulkan::VulkanDevice* vkDevice)
    : m_monitor(std::move(monitor)),
      m_platform(platform),
      m_vkDevice(vkDevice) 
{
}

MonitorContext::~MonitorContext() {
    stop();

    auto logicalDevice = m_vkDevice->getLogicalDevice();
    if (logicalDevice && logicalDevice->getHandle()) {
        vkDeviceWaitIdle(logicalDevice->getHandle());
        if (m_pipeline) vkDestroyPipeline(logicalDevice->getHandle(), m_pipeline, nullptr);
        if (m_pipelineLayout) vkDestroyPipelineLayout(logicalDevice->getHandle(), m_pipelineLayout, nullptr);
    }
}

bool MonitorContext::init() {
    spdlog::info("[MONITOR] Initializing context for {}", m_monitor->getName());

    m_wallpaperSurface = m_platform->createWallpaperSurface(m_monitor);
    if (m_wallpaperSurface) {
        m_wallpaperSurface->show();
    }

    m_renderContext = m_vkDevice->createContext();
    m_renderSurface = m_vkDevice->createSurface(m_wallpaperSurface.get());

    if (m_renderSurface) {
        m_renderSurface->build(m_monitor->getWidth(), m_monitor->getHeight());
    }

    auto logicalDevice = m_vkDevice->getLogicalDevice();

    m_uploadManager = std::make_shared<render::vulkan::VulkanUploadManager>(logicalDevice);
    m_uploadManager->init();
    
    m_extMemManager = std::make_shared<render::vulkan::VulkanExternalMemoryManager>(logicalDevice);

    m_descriptorAllocator = std::make_unique<render::vulkan::DescriptorAllocator>(logicalDevice);
    m_descriptorCache = std::make_unique<render::vulkan::DescriptorLayoutCache>(logicalDevice);

    m_mediaPlayer = std::make_shared<media::MediaPlayer>();
    m_mediaPlayer->setSupportedFormats({media::PixelFormat::RGBA8, media::PixelFormat::NV12, media::PixelFormat::YUV420P});
    m_mediaPlayer->setHardwareDecodeEnabled(core::system::CapabilityManager::instance().shouldUseHardwareDecode());

    return true;
}

void MonitorContext::loadWallpaper(const std::string& path) {
    if (!m_mediaPlayer) return;

    if (m_scheduler) {
        m_scheduler->stop();
    }

    auto logicalDevice = m_vkDevice->getLogicalDevice();
    vkDeviceWaitIdle(logicalDevice->getHandle());

    // Clean up old pipeline and layout
    if (m_pipeline) {
        vkDestroyPipeline(logicalDevice->getHandle(), m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout) {
        vkDestroyPipelineLayout(logicalDevice->getHandle(), m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    spdlog::info("[MONITOR] Loading wallpaper on {}: {}", m_monitor->getName(), path);
    auto source = std::make_shared<media::FileMediaSource>(path, media::MediaType::Video);
    m_mediaPlayer->load(source);
    m_mediaPlayer->play();

    std::shared_ptr<media::Frame> firstFrame = nullptr;
    while (!firstFrame) {
        firstFrame = m_mediaPlayer->getNextFrame();
        if (!firstFrame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    auto vkTexture = std::make_shared<render::vulkan::VulkanTexture>(logicalDevice);
    if (firstFrame->isHardwareBacked()) {
        render::vulkan::VulkanTextureUploader::importHardwareFrame(vkTexture, firstFrame, m_extMemManager.get());
    } else {
        render::vulkan::VulkanTextureUploader::upload(vkTexture, firstFrame, m_uploadManager.get());
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = vkTexture->getImageView();
    imageInfo.sampler = vkTexture->getSampler();

    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorLayout;
    
    auto builder = render::vulkan::DescriptorBuilder::begin(m_descriptorCache.get(), m_descriptorAllocator.get());
    
    if (vkTexture->isYuv()) {
        VkSampler immutableSampler = vkTexture->getSampler();
        builder.bindImageYcbcr(0, &imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &immutableSampler);
    } else {
        builder.bindImage(0, &imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    
    builder.build(descriptorSet, descriptorLayout);

    render::vulkan::ShaderLoader shaderLoader(logicalDevice);
    VkShaderModule vertShader = shaderLoader.loadSpv("shaders/fullscreen.vert.spv");
    VkShaderModule fragShader = shaderLoader.loadSpv("shaders/textured.frag.spv");

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(render::vulkan::FirstRenderNode::PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    vkCreatePipelineLayout(logicalDevice->getHandle(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);

    render::vulkan::PipelineBuilder pipelineBuilder;
    pipelineBuilder.addShaderStage(vertShader, VK_SHADER_STAGE_VERTEX_BIT)
                   .addShaderStage(fragShader, VK_SHADER_STAGE_FRAGMENT_BIT)
                   .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                   .setPolygonMode(VK_POLYGON_MODE_FILL)
                   .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                   .disableMultisampling()
                   .disableBlending()
                   .disableDepthTest()
                   .setColorAttachmentFormat(VK_FORMAT_B8G8R8A8_UNORM)
                   .setPipelineLayout(m_pipelineLayout);

    render::vulkan::VulkanPipelineCache pipelineCache(logicalDevice);
    pipelineCache.init("pipeline_cache_" + m_monitor->getId() + ".bin");

    m_pipeline = pipelineBuilder.build(logicalDevice, pipelineCache.getHandle());
    
    shaderLoader.destroyModule(vertShader);
    shaderLoader.destroyModule(fragShader);

    render::vulkan::FirstRenderNode::PushConstants pc;
    std::string scalingMode = core::SettingsManager::instance().getScalingMode(m_monitor->getId());
    
    float videoAspect = static_cast<float>(firstFrame->width) / static_cast<float>(firstFrame->height);
    float monitorAspect = static_cast<float>(m_monitor->getWidth()) / static_cast<float>(m_monitor->getHeight());
    
    if (scalingMode == "Fit") {
        if (monitorAspect > videoAspect) {
            pc.scaleX = videoAspect / monitorAspect;
            pc.offsetX = (1.0f - pc.scaleX) / 2.0f;
        } else {
            pc.scaleY = monitorAspect / videoAspect;
            pc.offsetY = (1.0f - pc.scaleY) / 2.0f;
        }
    } else if (scalingMode == "Crop") {
        if (monitorAspect > videoAspect) {
            pc.scaleY = monitorAspect / videoAspect;
            pc.offsetY = (1.0f - pc.scaleY) / 2.0f;
        } else {
            pc.scaleX = videoAspect / monitorAspect;
            pc.offsetX = (1.0f - pc.scaleX) / 2.0f;
        }
    }

    auto node = std::make_unique<render::vulkan::FirstRenderNode>(
        m_pipeline, 
        m_pipelineLayout, 
        descriptorSet, 
        pc,
        m_mediaPlayer,
        vkTexture,
        m_uploadManager.get(),
        m_extMemManager.get()
    );
    m_renderGraph = std::make_shared<render::RenderGraph>();
    m_renderGraph->addNode(std::move(node));

    m_scheduler = std::make_unique<render::RenderScheduler>(
        m_monitor, 
        m_wallpaperSurface.get(), 
        m_renderSurface.get(), 
        m_renderContext.get()
    );
    m_scheduler->setRenderGraph(m_renderGraph);

    pipelineCache.cleanup();
    start();
}

void MonitorContext::start() {
    if (m_scheduler) {
        m_scheduler->start();
    }
}

void MonitorContext::stop() {
    if (m_scheduler) {
        m_scheduler->stop();
    }
    if (m_mediaPlayer) {
        m_mediaPlayer->pause();
    }
}

void MonitorContext::pause() {
    if (m_mediaPlayer) {
        m_mediaPlayer->pause();
    }
}

void MonitorContext::resume() {
    if (m_mediaPlayer) {
        m_mediaPlayer->play();
    }
}

} // namespace luma::app::monitor
