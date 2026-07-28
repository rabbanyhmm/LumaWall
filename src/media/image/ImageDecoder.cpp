#include "ImageDecoder.hpp"
#include <core/Logging.hpp>
#include <stb_image.h>
#include <chrono>

namespace luma::media {

ImageDecoder::ImageDecoder(std::shared_ptr<IMediaSource> source, std::shared_ptr<FrameQueue> frameQueue)
    : m_source(std::move(source)), m_frameQueue(std::move(frameQueue)) {
}

ImageDecoder::~ImageDecoder() {
    stop();
}

bool ImageDecoder::init() {
    if (!m_source || m_source->getType() != MediaType::Image) {
        spdlog::error("[MEDIA] ImageDecoder requires an Image source");
        return false;
    }
    return true;
}

void ImageDecoder::start() {
    if (!m_running) {
        m_running = true;
        m_paused = false;
        m_decodeThread = std::thread(&ImageDecoder::decodeThreadFunc, this);
    } else {
        m_paused = false;
    }
}

void ImageDecoder::pause() {
    m_paused = true;
}

void ImageDecoder::stop() {
    m_running = false;
    if (m_decodeThread.joinable()) {
        m_decodeThread.join();
    }
}

void ImageDecoder::seek(double timestamp) {
    // Images don't have a timeline
}

void ImageDecoder::setSupportedFormats(const std::vector<PixelFormat>& formats) {
    // No-op for now
}

MediaCompatibilityReport ImageDecoder::getReport() const {
    MediaCompatibilityReport report;
    report.codec = "Static Image";
    report.decoderBackend = "stb_image";
    return report;
}

void ImageDecoder::decodeNextFrame() {
    if (m_decoded) return;
    
    std::string path = m_source->getURI();
    int width, height, channels;
    
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        spdlog::error("[MEDIA] ImageDecoder failed to load image: {}", path);
        m_decoded = true;
        return;
    }

    auto frame = std::make_shared<Frame>();
    frame->width = width;
    frame->height = height;
    frame->format = PixelFormat::RGBA8;
    frame->colorSpace = ColorSpace::SRGB;
    frame->timestamp = 0.0;
    frame->duration = 0.0; // Infinite duration for static image
    
    size_t dataSize = width * height * 4;
    frame->data.assign(pixels, pixels + dataSize);
    
    stbi_image_free(pixels);
    
    // Push blocks if queue is full
    m_frameQueue->push(frame);
    m_decoded = true;
    
    spdlog::info("[MEDIA] ImageDecoder successfully decoded: {}", path);
}

void ImageDecoder::decodeThreadFunc() {
    while (m_running) {
        if (m_paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (!m_decoded) {
            decodeNextFrame();
        } else {
            // Wait to prevent spinning, static image is done
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

} // namespace luma::media
