#include "HardwareDecodeManager.hpp"
#include <core/Logging.hpp>

namespace luma::media {

HardwareDecodeManager::~HardwareDecodeManager() {
    cleanup();
}

void HardwareDecodeManager::cleanup() {
    if (m_hwDeviceCtx) {
        av_buffer_unref(&m_hwDeviceCtx);
        m_hwDeviceCtx = nullptr;
    }
}

bool HardwareDecodeManager::init(AVCodecID codecId) {
    cleanup();

    const AVCodec* decoder = avcodec_find_decoder(codecId);
    if (!decoder) return false;

    // Ordered list of preferred hardware device types
    std::vector<AVHWDeviceType> preferredTypes = {
        AV_HWDEVICE_TYPE_VAAPI,
        AV_HWDEVICE_TYPE_CUDA,
        AV_HWDEVICE_TYPE_QSV,
        AV_HWDEVICE_TYPE_VDPAU
    };

    for (auto hwType : preferredTypes) {
        // Check if decoder supports this hardware type
        bool supported = false;
        for (int i = 0;; i++) {
            const AVCodecHWConfig* config = avcodec_get_hw_config(decoder, i);
            if (!config) break;
            
            if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == hwType) {
                m_hwPixFmt = config->pix_fmt;
                supported = true;
                break;
            }
        }

        if (supported) {
            int err = av_hwdevice_ctx_create(&m_hwDeviceCtx, hwType, nullptr, nullptr, 0);
            if (err >= 0) {
                spdlog::info("[MEDIA] Successfully initialized hardware decoder: {}", av_hwdevice_get_type_name(hwType));
                return true;
            } else {
                spdlog::warn("[MEDIA] Failed to initialize hardware decoder {}, trying next...", av_hwdevice_get_type_name(hwType));
            }
        }
    }

    spdlog::info("[MEDIA] No suitable hardware decoder found, falling back to software decode.");
    m_hwPixFmt = AV_PIX_FMT_NONE;
    return false;
}

AVPixelFormat HardwareDecodeManager::getFormatCallback(struct AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts) {
    const enum AVPixelFormat* p;
    HardwareDecodeManager* manager = static_cast<HardwareDecodeManager*>(ctx->opaque);

    if (manager && manager->getHardwarePixelFormat() != AV_PIX_FMT_NONE) {
        for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
            if (*p == manager->getHardwarePixelFormat()) {
                return *p;
            }
        }
    }
    
    // Fallback to software format if hardware format is not in list
    spdlog::warn("[MEDIA] Hardware pixel format not supported by decoder, falling back to software.");
    return pix_fmts[0];
}

} // namespace luma::media
