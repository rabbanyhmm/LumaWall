#include "VulkanTextureUploader.hpp"
#include <core/Logging.hpp>
#include <media/common/MediaTelemetry.hpp>
#include <chrono>
#include <cstring>

namespace luma::render::vulkan {

bool VulkanTextureUploader::upload(std::shared_ptr<VulkanTexture> texture, std::shared_ptr<media::Frame> frame, VulkanUploadManager* uploadManager) {
    if (!texture || !frame || !uploadManager) return false;

    auto startUpload = std::chrono::high_resolution_clock::now();

    if (frame->format == media::PixelFormat::NV12 || frame->format == media::PixelFormat::YUV420P) {
        VkFormat vkFormat = (frame->format == media::PixelFormat::NV12) ? VK_FORMAT_G8_B8R8_2PLANE_420_UNORM : VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
        
        if (!texture->createYuvImage(frame->width, frame->height, vkFormat)) {
            return false;
        }
        texture->updateYuvImage(frame, uploadManager);
    } else {
        if (!texture->loadFromFrame(frame, uploadManager)) {
            return false;
        }
    }

    auto endUpload = std::chrono::high_resolution_clock::now();
    double uploadTimeMs = std::chrono::duration<double, std::milli>(endUpload - startUpload).count();
    media::MediaTelemetry::getInstance().recordImportLatency(uploadTimeMs);

    return true;
}

bool VulkanTextureUploader::importHardwareFrame(std::shared_ptr<VulkanTexture> texture, std::shared_ptr<media::Frame> frame, VulkanExternalMemoryManager* extMemManager) {
    if (!texture || !frame || !extMemManager) return false;
    
    if (!frame->isHardwareBacked()) {
        spdlog::error("[VULKAN] Tried to import hardware frame, but frame has no hardware handle.");
        return false;
    }
    
    auto startImport = std::chrono::high_resolution_clock::now();

    // Pass the hardware handle abstractly to the Vulkan memory manager
    VkImage importedImage = VK_NULL_HANDLE;
    if (extMemManager->importHardwareFrame(frame, importedImage, texture.get())) {
        // Texture would be fully constructed with the imported image
        auto endImport = std::chrono::high_resolution_clock::now();
        double importTimeMs = std::chrono::duration<double, std::milli>(endImport - startImport).count();
        media::MediaTelemetry::getInstance().recordImportLatency(importTimeMs);
        return true;
    }
    
    return false;
}

} // namespace luma::render::vulkan
