#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include "FFmpegPacketQueue.hpp"
#include "FFmpegFrameConverter.hpp"
#include <media/common/FrameQueue.hpp>
#include <media/hardware/HardwareDecodeManager.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace luma::media {

class FFmpegVideoDecoder {
public:
    FFmpegVideoDecoder();
    ~FFmpegVideoDecoder();

    bool open(AVStream* stream);
    void start(std::shared_ptr<FFmpegPacketQueue> packetQueue, std::shared_ptr<FrameQueue> frameQueue);
    void stop();
    void flush();
    void setSupportedFormats(const std::vector<PixelFormat>& formats) { m_converter->setSupportedFormats(formats); }
    void setHardwareDecodeEnabled(bool enabled) { m_hwDecodeEnabled = enabled; }

    std::string getCodecName() const { 
        return m_codecCtx ? (avcodec_get_name(m_codecCtx->codec_id) ? avcodec_get_name(m_codecCtx->codec_id) : "Unknown") : "Unknown"; 
    }
    uint32_t getWidth() const { return m_codecCtx ? m_codecCtx->width : 0; }
    uint32_t getHeight() const { return m_codecCtx ? m_codecCtx->height : 0; }
    bool isHardwareAccelerated() const { return m_codecCtx && m_codecCtx->hw_device_ctx != nullptr; }

private:
    void decodeLoop();
    void flushCodec();

    AVCodecContext* m_codecCtx{nullptr};
    AVStream* m_stream{nullptr};
    
    std::shared_ptr<HardwareDecodeManager> m_hwManager;
    std::shared_ptr<FFmpegFrameConverter> m_converter;

    std::shared_ptr<FFmpegPacketQueue> m_packetQueue;
    std::shared_ptr<FrameQueue> m_frameQueue;

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    bool m_hwDecodeEnabled{true};
};

} // namespace luma::media
