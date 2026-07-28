#pragma once
#include <memory>
#include <media/common/Frame.hpp>
#include <media/common/IFrameProvider.hpp>
#include <media/common/MediaCompatibilityReport.hpp>

namespace luma::media {

class IMediaDecoder {
public:
    virtual ~IMediaDecoder() = default;

    virtual bool init() = 0;
    virtual void start() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void seek(double timestamp) = 0;
    virtual void setSupportedFormats(const std::vector<PixelFormat>& formats) = 0;
    virtual void setHardwareDecodeEnabled(bool enabled) {}
    virtual MediaCompatibilityReport getReport() const = 0;

    // Pushes decoded frames to the FrameQueue. This is usually called internally by the decode thread.
    virtual void decodeNextFrame() = 0; 
};

} // namespace luma::media
