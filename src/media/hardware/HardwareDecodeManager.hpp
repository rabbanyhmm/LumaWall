#pragma once
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
}

namespace luma::media {

class HardwareDecodeManager {
public:
    HardwareDecodeManager() = default;
    ~HardwareDecodeManager();

    bool init(AVCodecID codecId);
    void cleanup();

    AVBufferRef* getDeviceContext() const { return m_hwDeviceCtx; }
    AVPixelFormat getHardwarePixelFormat() const { return m_hwPixFmt; }
    
    // Callback to assign to AVCodecContext::get_format
    static AVPixelFormat getFormatCallback(struct AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts);

private:
    AVBufferRef* m_hwDeviceCtx{nullptr};
    AVPixelFormat m_hwPixFmt{AV_PIX_FMT_NONE};
};

} // namespace luma::media
