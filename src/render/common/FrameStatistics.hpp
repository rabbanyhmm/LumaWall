#pragma once
#include <cstdint>

namespace luma::render {

struct FrameStatistics {
    double cpuFrameTimeMs{0.0};
    double gpuFrameTimeMs{0.0};
    double fps{0.0};
    double presentLatencyMs{0.0};
    
    uint32_t droppedFrames{0};
    uint32_t swapchainRecreations{0};
    uint64_t uploadBandwidthBytes{0};
    uint64_t vramUsageBytes{0};
};

} // namespace luma::render
