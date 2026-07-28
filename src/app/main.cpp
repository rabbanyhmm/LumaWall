#include <iostream>
#include <core/Engine.hpp>
#include <core/events/EventBus.hpp>
#include <platform/PlatformFactory.hpp>
#include <platform/x11/X11PlatformBackend.hpp>
#include <platform/common/IMonitor.hpp>
#include <thread>
#include <chrono>
#include <render/vulkan/VulkanBackend.hpp>
#include <render/common/BackendRegistry.hpp>
#include <render/common/RenderScheduler.hpp>
#include <render/common/RenderGraph.hpp>
#include <render/vulkan/VulkanDevice.hpp>
#include <render/vulkan/shader/ShaderLoader.hpp>
#include <render/vulkan/pipeline/PipelineBuilder.hpp>
#include <render/vulkan/descriptor/DescriptorAllocator.hpp>
#include <render/vulkan/descriptor/DescriptorLayoutCache.hpp>
#include <render/vulkan/descriptor/DescriptorBuilder.hpp>
#include <media/common/MediaTelemetry.hpp>
#include <library/DatabaseManager.hpp>
#include <library/PlaylistEngine.hpp>
#include "DBusServer.hpp"
#include <QCoreApplication>
#include <csignal>
#include <atomic>
#include <core/SettingsManager.hpp>
#include <core/task/TaskManager.hpp>
#include <core/system/CapabilityManager.hpp>
#include "monitor/MonitorManager.hpp"
#include <spdlog/spdlog.h>
#include <platform/gnome/GnomeIntegration.hpp>

std::atomic<bool> g_running{true};

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    g_running = false;
}
#include <render/vulkan/texture/VulkanTexture.hpp>
#include <render/vulkan/texture/VulkanTextureUploader.hpp>
#include <render/vulkan/command/CommandPool.hpp>
#include <render/vulkan/upload/VulkanUploadManager.hpp>
#include <render/vulkan/FirstRenderNode.hpp>
#include <render/vulkan/pipeline/VulkanPipelineCache.hpp>
#include <render/vulkan/texture/TextureManager.hpp>
#include <media/scheduler/MediaPlayer.hpp>
#include <media/common/FileMediaSource.hpp>
#include <media/capabilities/MediaCapabilityDetector.hpp>

class DummyMonitor : public luma::platform::IMonitor {
public:
    std::string getId() const override { return "0"; }
    std::string getName() const override { return "Dummy"; }
    uint32_t getWidth() const override { return 1920; }
    uint32_t getHeight() const override { return 1080; }
    uint32_t getRefreshRate() const override { return 60; }
    float getScaleFactor() const override { return 1.0f; }
};

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::string videoPath = "/tmp/test.mp4";
    bool benchmarkMode = false;
    bool daemonMode = false;
    int durationMs = 5000;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--video" && i + 1 < argc) {
            videoPath = argv[++i];
        } else if (arg == "--benchmark") {
            benchmarkMode = true;
        } else if (arg == "--daemon") {
            daemonMode = true;
        } else if (arg == "--duration" && i + 1 < argc) {
            durationMs = std::stoi(argv[++i]);
        } else if (arg == "--idle") {
            // Already handled / placeholder for headless loop testing
        }
    }

    auto eventBus = std::make_shared<luma::core::events::EventBus>();
    luma::core::SettingsManager::instance().initialize();
    luma::core::task::TaskManager::instance().start();
    
    auto platform = luma::platform::PlatformFactory::create();
    
    if (!platform || !platform->init(eventBus)) {
        spdlog::warn("[PLATFORM] Primary backend failed to initialize, falling back to X11/XWayland backend...");
        platform = std::make_unique<luma::platform::x11::X11PlatformBackend>();
        if (!platform || !platform->init(eventBus)) {
            std::cerr << "Failed to initialize platform backend.\n";
            return 1;
        }
    }

    auto vkBackend = std::make_shared<luma::render::vulkan::VulkanBackend>();
    vkBackend->init();
    auto renderDevice = vkBackend->createDevice();
    auto vkDevice = dynamic_cast<luma::render::vulkan::VulkanDevice*>(renderDevice.get());

    if (vkDevice) {
        // Initialize Capabilities
        auto& capManager = luma::core::system::CapabilityManager::instance();
        auto mediaCaps = luma::media::MediaCapabilityDetector::detect();
        
        capManager.setHardwareDecode(mediaCaps.hasHardwareAcceleration);
        capManager.setZeroCopy(mediaCaps.hasHardwareAcceleration); // Assume zero copy if HW decode works for now
        capManager.setYCbCr(true); // Hardcoded true for now if Vulkan supports it
        capManager.setTimelineSemaphores(true);
        capManager.setDynamicRendering(true);

        capManager.detectCapabilities("Vulkan 1.3", "AMD/NVIDIA/Intel"); // Use dummy string for now

        // Initialize Library and Playlist
        auto db = std::make_shared<luma::library::DatabaseManager>();
        db->init(std::string(std::getenv("HOME")) + "/.local/share/lumawall/library.db");
        
        auto playlistEngine = std::make_shared<luma::library::PlaylistEngine>(db);
        playlistEngine->loadAllWallpapers();

        // Start Monitor Manager
        auto monitorManager = std::make_shared<luma::app::monitor::MonitorManager>(platform.get(), vkDevice, eventBus);
        monitorManager->loadSettingsAndRestore();

        // Start D-Bus Server
        auto dbusServer = std::make_shared<luma::app::DBusServer>(playlistEngine);
        dbusServer->registerService();
        
        // Connect DBus signals to Monitor Manager
        QObject::connect(dbusServer.get(), &luma::app::DBusServer::requestPlayFile, [&monitorManager](const QString& monitorId, const QString& path) {
            monitorManager->playOnMonitor(monitorId.toStdString(), path.toStdString());
        });
        QObject::connect(dbusServer.get(), &luma::app::DBusServer::requestPause, [&monitorManager](const QString& monitorId) {
            monitorManager->pauseMonitor(monitorId.toStdString());
        });

        // Initialize Desktop Integration (GNOME for now)
        auto gnomeIntegration = std::make_shared<luma::platform::gnome::GnomeIntegration>(
            dbusServer,
            [&monitorManager](bool pause) {
                // Pause all monitors if true, resume if false
                // For now, we only have one monitor (DP-1), or we can just pause the main one
                if (pause) monitorManager->pauseMonitor(""); 
                // We'd need a resumeMonitor method if pause is false, but currently PauseMonitor is implemented.
                // Assuming we'll add resume logic later or just let the user resume via DBus.
            }
        );
        gnomeIntegration->initialize();

        if (daemonMode) {
            std::cout << "Running in Daemon Mode...\n";
            while (g_running) {
                QCoreApplication::processEvents();
                platform->pumpEvents();
                monitorManager->processEvents();
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        } else {
            std::cout << "Starting Render Loop for " << durationMs << " ms...\n";
            int loopCount = durationMs / 100;
            for (int i = 0; i < loopCount && g_running; ++i) {
                QCoreApplication::processEvents();
                platform->pumpEvents();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
        // Enforce strict deterministic shutdown order
        vkDeviceWaitIdle(vkDevice->getLogicalDevice()->getHandle());
        luma::core::task::TaskManager::instance().stop();
    } else {
        std::cout << "LumaWall initialized successfully (No Vulkan).\n";
    }

    return 0;
}
