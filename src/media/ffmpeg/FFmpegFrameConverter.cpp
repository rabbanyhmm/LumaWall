#include "FFmpegFrameConverter.hpp"
#include <core/Logging.hpp>
#include <cstring>
#include <media/common/DmaBufFrame.hpp>
extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_drm.h>
}

namespace luma::media {

FFmpegFrameConverter::~FFmpegFrameConverter() {
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
    }
}

std::shared_ptr<Frame> FFmpegFrameConverter::convert(AVFrame* avFrame, double timeBase) {
    if (!avFrame) return nullptr;

    auto frame = std::make_shared<Frame>();
    frame->width = avFrame->width;
    frame->height = avFrame->height;
    
    // Calculate timestamp and duration
    if (avFrame->pts != AV_NOPTS_VALUE) {
        frame->timestamp = avFrame->pts * timeBase;
    }
    
    if (avFrame->duration > 0) {
        frame->duration = avFrame->duration * timeBase;
    } else {
        frame->duration = 0.0; // Needs to be guessed by demuxer if missing
    }

    // Determine format
    AVPixelFormat pixFmt = static_cast<AVPixelFormat>(avFrame->format);
    
    // Fast path: Hardware Decode Zero-Copy
    if (pixFmt == AV_PIX_FMT_VAAPI) {
        AVFrame* drmFrame = av_frame_alloc();
        drmFrame->format = AV_PIX_FMT_DRM_PRIME;
        
        if (av_hwframe_map(drmFrame, avFrame, 0) == 0) {
            auto avDrmDesc = reinterpret_cast<const AVDRMFrameDescriptor*>(drmFrame->data[0]);
            if (avDrmDesc) {
                std::vector<DmaBufPlane> planes;
                for (int i = 0; i < avDrmDesc->nb_layers; ++i) {
                    const AVDRMLayerDescriptor& layer = avDrmDesc->layers[i];
                    for (int j = 0; j < layer.nb_planes; ++j) {
                        const AVDRMPlaneDescriptor& plane = layer.planes[j];
                        const AVDRMObjectDescriptor& object = avDrmDesc->objects[plane.object_index];
                        
                        DmaBufPlane dmaPlane;
                        dmaPlane.fd = object.fd;
                        dmaPlane.offset = plane.offset;
                        dmaPlane.pitch = plane.pitch;
                        planes.push_back(dmaPlane);
                    }
                }
                
                uint64_t modifier = avDrmDesc->objects[0].format_modifier;
                
                std::shared_ptr<AVFrame> sharedDrmFrame(drmFrame, [](AVFrame* f) {
                    av_frame_unref(f);
                    av_frame_free(&f);
                });
                
                auto dmaFrame = std::make_shared<DmaBufFrame>(sharedDrmFrame, modifier, planes);
                frame->hwFrame = dmaFrame;
                
                // Assume NV12 for VAAPI decoding for now. Can be expanded by reading avDrmDesc->layers[0].format
                frame->format = PixelFormat::NV12; 
                return frame;
            }
            av_frame_free(&drmFrame);
        } else {
            spdlog::error("[MEDIA] Failed to map VAAPI frame to DRM PRIME");
            av_frame_free(&drmFrame);
        }
    }

    PixelFormat targetFormat = PixelFormat::UNKNOWN;
    
    if (pixFmt == AV_PIX_FMT_NV12) targetFormat = PixelFormat::NV12;
    else if (pixFmt == AV_PIX_FMT_YUV420P) targetFormat = PixelFormat::YUV420P;
    else if (pixFmt == AV_PIX_FMT_RGBA) targetFormat = PixelFormat::RGBA8;

    bool supported = false;
    for (auto f : m_supportedFormats) {
        if (f == targetFormat) {
            supported = true;
            break;
        }
    }

    if (!supported || targetFormat == PixelFormat::UNKNOWN) {
        if (!m_swsCtx || m_lastWidth != avFrame->width || m_lastHeight != avFrame->height || m_lastFormat != pixFmt) {
            if (m_swsCtx) sws_freeContext(m_swsCtx);
            
            m_swsCtx = sws_getContext(
                avFrame->width, avFrame->height, pixFmt,
                avFrame->width, avFrame->height, AV_PIX_FMT_RGBA,
                SWS_BILINEAR, nullptr, nullptr, nullptr
            );
            
            m_lastWidth = avFrame->width;
            m_lastHeight = avFrame->height;
            m_lastFormat = pixFmt;
        }
        
        if (!m_swsCtx) {
            spdlog::error("[MEDIA] Failed to create sws context for RGBA conversion");
            return nullptr;
        }

        frame->format = PixelFormat::RGBA8;
        size_t size = avFrame->width * avFrame->height * 4;
        frame->data.resize(size);
        
        uint8_t* destData[4] = { frame->data.data(), nullptr, nullptr, nullptr };
        int destLinesize[4] = { avFrame->width * 4, 0, 0, 0 };
        
        sws_scale(m_swsCtx, avFrame->data, avFrame->linesize, 0, avFrame->height, destData, destLinesize);
        
    } else {
        if (pixFmt == AV_PIX_FMT_NV12) {
            frame->format = PixelFormat::NV12;
            
            // Plane 0: Y
            size_t ySize = avFrame->width * avFrame->height;
            frame->data.resize(ySize);
            if (avFrame->linesize[0] == avFrame->width) {
                memcpy(frame->data.data(), avFrame->data[0], ySize);
            } else {
                for (uint32_t i = 0; i < avFrame->height; ++i) {
                    memcpy(frame->data.data() + i * avFrame->width, avFrame->data[0] + i * avFrame->linesize[0], avFrame->width);
                }
            }
            
            // Plane 1: UV
            size_t uvSize = (avFrame->width / 2) * (avFrame->height / 2) * 2;
            frame->dataPlane1.resize(uvSize);
            uint32_t uvWidth = avFrame->width;
            if (avFrame->linesize[1] == (int)uvWidth) {
                memcpy(frame->dataPlane1.data(), avFrame->data[1], uvSize);
            } else {
                for (uint32_t i = 0; i < avFrame->height / 2; ++i) {
                    memcpy(frame->dataPlane1.data() + i * uvWidth, avFrame->data[1] + i * avFrame->linesize[1], uvWidth);
                }
            }
            
        } else if (pixFmt == AV_PIX_FMT_YUV420P) {
            frame->format = PixelFormat::YUV420P;
            
            // Plane 0: Y
            size_t ySize = avFrame->width * avFrame->height;
            frame->data.resize(ySize);
            for (uint32_t i = 0; i < avFrame->height; ++i) {
                memcpy(frame->data.data() + i * avFrame->width, avFrame->data[0] + i * avFrame->linesize[0], avFrame->width);
            }
            
            // Plane 1: U
            size_t uSize = (avFrame->width / 2) * (avFrame->height / 2);
            frame->dataPlane1.resize(uSize);
            for (uint32_t i = 0; i < avFrame->height / 2; ++i) {
                memcpy(frame->dataPlane1.data() + i * (avFrame->width / 2), avFrame->data[1] + i * avFrame->linesize[1], avFrame->width / 2);
            }
            
            // Plane 2: V
            size_t vSize = (avFrame->width / 2) * (avFrame->height / 2);
            frame->dataPlane2.resize(vSize);
            for (uint32_t i = 0; i < avFrame->height / 2; ++i) {
                memcpy(frame->dataPlane2.data() + i * (avFrame->width / 2), avFrame->data[2] + i * avFrame->linesize[2], avFrame->width / 2);
            }
            
        } else if (pixFmt == AV_PIX_FMT_RGBA) {
            frame->format = PixelFormat::RGBA8;
            size_t size = avFrame->width * avFrame->height * 4;
            frame->data.resize(size);
            for (uint32_t i = 0; i < avFrame->height; ++i) {
                memcpy(frame->data.data() + i * avFrame->width * 4, avFrame->data[0] + i * avFrame->linesize[0], avFrame->width * 4);
            }
        }
    }

    return frame;
}

} // namespace luma::media
