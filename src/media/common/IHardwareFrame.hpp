#pragma once

namespace luma::media {

enum class HardwareFrameType {
    None,
    DmaBuf,
    VaApi,
    VulkanVideo,
    Cuda,
    D3D11,
    Metal
};

class IHardwareFrame {
public:
    virtual ~IHardwareFrame() = default;

    virtual HardwareFrameType getType() const = 0;

    // Returns an opaque native handle (e.g., DMABUF FD array or AVHWFramesContext pointer)
    // The specific renderer backend (like VulkanExternalMemoryManager) will cast and use this.
    virtual void* getNativeHandle() const = 0;

    // Expose lifetime management if the decoder needs to be explicitly notified when the GPU is done
    virtual void release() = 0;
};

} // namespace luma::media
