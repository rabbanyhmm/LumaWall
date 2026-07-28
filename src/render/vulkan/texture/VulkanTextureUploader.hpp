#pragma once
#include <memory>
#include <render/vulkan/texture/VulkanTexture.hpp>
#include <render/vulkan/upload/VulkanUploadManager.hpp>
#include <render/vulkan/memory/VulkanExternalMemoryManager.hpp>
#include <render/common/FrameImportManager.hpp>
#include <media/common/Frame.hpp>

namespace luma::render::vulkan {

class VulkanTextureUploader : public FrameImportManager {
public:
    // Standard CPU-backed frame upload
    static bool upload(std::shared_ptr<VulkanTexture> texture, std::shared_ptr<media::Frame> frame, VulkanUploadManager* uploadManager);

    // Hardware-backed DMABUF import
    static bool importHardwareFrame(std::shared_ptr<VulkanTexture> texture, std::shared_ptr<media::Frame> frame, VulkanExternalMemoryManager* extMemManager);

    // Implements FrameImportManager interface
    bool importFrame(std::shared_ptr<media::Frame> frame) override {
        // We will tie this to the active texture/managers later
        return false; 
    }
};

} // namespace luma::render::vulkan
