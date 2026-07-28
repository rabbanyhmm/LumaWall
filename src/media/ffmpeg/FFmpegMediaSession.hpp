#pragma once
#include <media/common/IMediaDecoder.hpp>
#include <media/common/IMediaSource.hpp>
#include <media/common/FrameQueue.hpp>
#include "FFmpegDemuxer.hpp"
#include "FFmpegVideoDecoder.hpp"
#include "FFmpegAudioDecoder.hpp"
#include "FFmpegPacketQueue.hpp"
#include <memory>

namespace luma::media {

class FFmpegMediaSession : public IMediaDecoder {
public:
    FFmpegMediaSession(std::shared_ptr<IMediaSource> source, std::shared_ptr<FrameQueue> frameQueue);
    ~FFmpegMediaSession() override;

    bool init() override;
    void start() override;
    void pause() override;
    void stop() override;
    void seek(double timestamp) override;
    void setSupportedFormats(const std::vector<PixelFormat>& formats) override;
    void setHardwareDecodeEnabled(bool enabled) override;
    MediaCompatibilityReport getReport() const override;
    void decodeNextFrame() override;

private:
    std::shared_ptr<IMediaSource> m_source;
    std::shared_ptr<FrameQueue> m_frameQueue;

    std::shared_ptr<FFmpegPacketQueue> m_videoPacketQueue;
    std::shared_ptr<FFmpegPacketQueue> m_audioPacketQueue;

    std::unique_ptr<FFmpegDemuxer> m_demuxer;
    std::unique_ptr<FFmpegVideoDecoder> m_videoDecoder;
    std::unique_ptr<FFmpegAudioDecoder> m_audioDecoder;
};

} // namespace luma::media
