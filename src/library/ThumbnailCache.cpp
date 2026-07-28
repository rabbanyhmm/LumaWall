#include "ThumbnailCache.hpp"
#include <core/Logging.hpp>
#include <filesystem>
#include <QImage>
#include <functional>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace luma::library {

ThumbnailCache::ThumbnailCache() {
    const char* home = std::getenv("HOME");
    m_cacheDir = home ? std::string(home) + "/.cache/lumawall/thumbnails" : "/tmp/lumawall/thumbnails";
    std::filesystem::create_directories(m_cacheDir);
}

ThumbnailCache::~ThumbnailCache() = default;

std::string ThumbnailCache::computeCacheKey(const std::string& videoPath) const {
    try {
        auto lastWrite = std::filesystem::last_write_time(videoPath);
        std::string hashInput = videoPath + std::to_string(lastWrite.time_since_epoch().count());
        size_t hash = std::hash<std::string>{}(hashInput);
        return std::to_string(hash);
    } catch (...) {
        return std::to_string(std::hash<std::string>{}(videoPath));
    }
}

std::optional<std::string> ThumbnailCache::getThumbnail(const std::string& videoPath) {
    std::string key = computeCacheKey(videoPath);
    std::string outPath = m_cacheDir + "/" + key + ".jpg";

    if (std::filesystem::exists(outPath)) {
        return outPath;
    }

    if (generateThumbnail(videoPath, outPath)) {
        return outPath;
    }

    return std::nullopt;
}

bool ThumbnailCache::generateThumbnail(const std::string& videoPath, const std::string& outPath) {
    AVFormatContext* formatCtx = nullptr;
    if (avformat_open_input(&formatCtx, videoPath.c_str(), nullptr, nullptr) < 0) {
        return false;
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        avformat_close_input(&formatCtx);
        return false;
    }

    int videoStreamIdx = -1;
    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIdx = i;
            break;
        }
    }

    if (videoStreamIdx == -1) {
        avformat_close_input(&formatCtx);
        return false;
    }

    AVStream* stream = formatCtx->streams[videoStreamIdx];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        avformat_close_input(&formatCtx);
        return false;
    }

    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, stream->codecpar);
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    // Seek to ~15% of the video
    if (formatCtx->duration != AV_NOPTS_VALUE) {
        int64_t targetTimestamp = (formatCtx->duration * 15) / 100;
        avformat_seek_file(formatCtx, -1, INT64_MIN, targetTimestamp, INT64_MAX, AVSEEK_FLAG_BACKWARD);
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* rgbFrame = av_frame_alloc();

    bool frameDecoded = false;
    int maxAttempts = 30; // Read a few packets to get a frame after seeking
    while (maxAttempts-- > 0 && av_read_frame(formatCtx, packet) >= 0) {
        if (packet->stream_index == videoStreamIdx) {
            if (avcodec_send_packet(codecCtx, packet) == 0) {
                if (avcodec_receive_frame(codecCtx, frame) == 0) {
                    frameDecoded = true;
                    av_packet_unref(packet);
                    break;
                }
            }
        }
        av_packet_unref(packet);
    }

    bool success = false;
    if (frameDecoded) {
        int targetWidth = 320;
        int targetHeight = (frame->height * targetWidth) / frame->width;

        rgbFrame->format = AV_PIX_FMT_RGBA;
        rgbFrame->width = targetWidth;
        rgbFrame->height = targetHeight;
        av_frame_get_buffer(rgbFrame, 32);

        SwsContext* swsCtx = sws_getContext(
            frame->width, frame->height, codecCtx->pix_fmt,
            targetWidth, targetHeight, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );

        if (swsCtx) {
            sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, rgbFrame->data, rgbFrame->linesize);

            // Use Qt to save the image
            QImage image(rgbFrame->data[0], targetWidth, targetHeight, rgbFrame->linesize[0], QImage::Format_RGBA8888);
            if (image.save(QString::fromStdString(outPath), "JPG")) {
                success = true;
            } else {
                spdlog::error("[LIBRARY] Failed to save QImage to {}", outPath);
            }

            sws_freeContext(swsCtx);
        }
    }

    av_frame_free(&rgbFrame);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);

    return success;
}

} // namespace luma::library
