#pragma once
#include <media/common/IMediaPlayer.hpp>
#include <media/common/IMediaSource.hpp>
#include <media/common/IMediaDecoder.hpp>
#include <media/common/FrameQueue.hpp>
#include <media/common/Clock.hpp>
#include <memory>
#include <vector>

namespace luma::media {

class MediaPlayer : public IMediaPlayer {
public:
    MediaPlayer();
    ~MediaPlayer() override;

    void load(std::shared_ptr<IMediaSource> source) override;
    
    void play() override;
    void pause() override;
    void stop() override;
    void seek(double timestamp) override;
    
    void setPlaybackSpeed(float speed) override;
    void setLooping(bool loop) override;
    void setSupportedFormats(const std::vector<PixelFormat>& formats) override;
    void setHardwareDecodeEnabled(bool enabled) override;
    MediaCompatibilityReport getReport() const override;

    std::shared_ptr<Frame> getNextFrame() override;

private:
    std::shared_ptr<IMediaSource> m_source;
    std::shared_ptr<IMediaDecoder> m_decoder;
    std::shared_ptr<FrameQueue> m_frameQueue;
    Clock m_clock;

    std::shared_ptr<Frame> m_currentFrame;
    bool m_looping{true};
    bool m_hwDecodeEnabled{true};
    std::vector<PixelFormat> m_supportedFormats;
};

} // namespace luma::media
