#pragma once
#include <media/common/IMediaDecoder.hpp>
#include <media/common/IMediaSource.hpp>
#include <media/common/FrameQueue.hpp>
#include <string>
#include <thread>
#include <atomic>

namespace luma::media {

class ImageDecoder : public IMediaDecoder {
public:
    ImageDecoder(std::shared_ptr<IMediaSource> source, std::shared_ptr<FrameQueue> frameQueue);
    ~ImageDecoder() override;

    bool init() override;
    void start() override;
    void pause() override;
    void stop() override;
    void seek(double timestamp) override;
    void setSupportedFormats(const std::vector<PixelFormat>& formats) override;
    MediaCompatibilityReport getReport() const override;
    void decodeNextFrame() override;

private:
    void decodeThreadFunc();

    std::shared_ptr<IMediaSource> m_source;
    std::shared_ptr<FrameQueue> m_frameQueue;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    std::thread m_decodeThread;
    
    bool m_decoded{false}; // static image only needs to decode once
};

} // namespace luma::media
