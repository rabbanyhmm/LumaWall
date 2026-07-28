#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <functional>
#include <render/vulkan/device/VulkanLogicalDevice.hpp>

namespace luma::render::vulkan {

struct UploadContext {
    VkFence uploadFence;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
};

class VulkanUploadManager {
public:
    VulkanUploadManager(std::shared_ptr<VulkanLogicalDevice> logicalDevice);
    ~VulkanUploadManager();

    bool init();
    void cleanup();

    // Allocate space from the ring buffer.
    // Blocks if there isn't enough space until previous uploads finish.
    struct StagingAllocation {
        VkBuffer buffer;
        size_t offset;
        void* mappedData;
    };

    StagingAllocation allocateStagingBuffer(size_t size);
    
    // Submits the upload command buffer. If 'wait' is true, blocks until complete.
    void submitUploads(std::function<void(VkCommandBuffer cmd)>&& function, bool wait = false);

private:
    std::shared_ptr<VulkanLogicalDevice> m_logicalDevice;
    UploadContext m_uploadContext;
    
    VkBuffer m_stagingBuffer{VK_NULL_HANDLE};
    VmaAllocation m_stagingAllocation{VK_NULL_HANDLE};
    void* m_mappedData{nullptr};
    
    size_t m_ringSize = 32 * 1024 * 1024; // 32MB staging ring buffer
    size_t m_head = 0;
    size_t m_tail = 0;
};

} // namespace luma::render::vulkan
