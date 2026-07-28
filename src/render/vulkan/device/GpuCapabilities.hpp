#pragma once
#include <string>
#include <cstdint>

namespace luma::render::vulkan {

struct GpuCapabilities {
    std::string deviceName;
    uint32_t apiVersion{0};
    uint32_t driverVersion{0};

    // Features
    bool supportsDynamicRendering{false};
    bool supportsTimelineSemaphores{false};
    bool supportsDescriptorIndexing{false};
    bool supportsSynchronization2{false};

    // Queues
    bool hasDedicatedTransferQueue{false};

    // External Memory
    bool supportsDmaBufImport{false};

    // Limits
    uint32_t maxTextureDimension2D{0};
    uint32_t maxFramebufferWidth{0};
    uint32_t maxFramebufferHeight{0};
};

} // namespace luma::render::vulkan
