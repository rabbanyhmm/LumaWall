#pragma once
#include <memory>
#include <media/common/Frame.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace luma::media {

class FFmpegFrameConverter {
public:
    FFmpegFrameConverter() = default;
    ~FFmpegFrameConverter();

    void setSupportedFormats(const std::vector<PixelFormat>& formats) { m_supportedFormats = formats; }

    std::shared_ptr<Frame> convert(AVFrame* avFrame, double timeBase);

private:
    std::vector<PixelFormat> m_supportedFormats{PixelFormat::RGBA8};
    SwsContext* m_swsCtx{nullptr};
    int m_lastWidth{0};
    int m_lastHeight{0};
    AVPixelFormat m_lastFormat{AV_PIX_FMT_NONE};
};

} // namespace luma::media
