#pragma once
#include <memory>
#include <media/common/IMediaSource.hpp>
#include <media/common/IFrameProvider.hpp>
#include <media/common/MediaCompatibilityReport.hpp>

namespace luma::media {

class IMediaPlayer : public IFrameProvider {
public:
    virtual ~IMediaPlayer() = default;

    virtual void load(std::shared_ptr<IMediaSource> source) = 0;
    
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void seek(double timestamp) = 0;
    
    virtual void setPlaybackSpeed(float speed) = 0;
    virtual void setLooping(bool loop) = 0;
    virtual void setSupportedFormats(const std::vector<PixelFormat>& formats) = 0;
    virtual void setHardwareDecodeEnabled(bool enabled) = 0;
    virtual MediaCompatibilityReport getReport() const = 0;
};

} // namespace luma::media
