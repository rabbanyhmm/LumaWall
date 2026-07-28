#include "MediaTelemetry.hpp"
#include <numeric>
#include <fstream>
#include <core/Logging.hpp>

namespace luma::media {

void MediaTelemetry::recordDecodeLatency(double ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_decodeLatencies.push_back(ms);
}

void MediaTelemetry::recordImportLatency(double ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_importLatencies.push_back(ms);
}

void MediaTelemetry::recordPresentationLatency(double ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_presentationLatencies.push_back(ms);
}

void MediaTelemetry::recordCpuUsage(double percent) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cpuUsages.push_back(percent);
}

void MediaTelemetry::recordGpuRenderTime(double ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_gpuRenderTimes.push_back(ms);
}

void MediaTelemetry::setPlaybackInfo(const std::string& codec, const std::string& resolution, 
                                     const std::string& pixelFormat, const std::string& decoderBackend,
                                     bool hardwareAccelerated, bool zeroCopy, const std::string& uploadPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_codec = codec;
    m_resolution = resolution;
    m_pixelFormat = pixelFormat;
    m_decoderBackend = decoderBackend;
    m_hardwareAccelerated = hardwareAccelerated;
    m_zeroCopy = zeroCopy;
    m_uploadPath = uploadPath;
}

void MediaTelemetry::setFallbackReason(const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fallbackReason = reason;
}

double MediaTelemetry::calculateAverage(const std::vector<double>& data) const {
    if (data.empty()) return 0.0;
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    return sum / data.size();
}

std::string MediaTelemetry::generateCompatibilityReport() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string report = "{\n";
    report += "  \"Codec\": \"" + m_codec + "\",\n";
    report += "  \"Resolution\": \"" + m_resolution + "\",\n";
    report += "  \"PixelFormat\": \"" + m_pixelFormat + "\",\n";
    report += "  \"DecoderBackend\": \"" + m_decoderBackend + "\",\n";
    report += "  \"HardwareAcceleration\": " + std::string(m_hardwareAccelerated ? "true" : "false") + ",\n";
    report += "  \"ZeroCopy\": " + std::string(m_zeroCopy ? "true" : "false") + ",\n";
    report += "  \"UploadPath\": \"" + m_uploadPath + "\",\n";
    
    if (!m_fallbackReason.empty()) {
        report += "  \"FallbackReason\": \"" + m_fallbackReason + "\",\n";
    }

    report += "  \"Metrics\": {\n";
    report += "    \"AverageDecodeLatencyMs\": " + std::to_string(calculateAverage(m_decodeLatencies)) + ",\n";
    report += "    \"AverageImportLatencyMs\": " + std::to_string(calculateAverage(m_importLatencies)) + ",\n";
    report += "    \"AveragePresentationLatencyMs\": " + std::to_string(calculateAverage(m_presentationLatencies)) + ",\n";
    report += "    \"AverageGpuRenderTimeMs\": " + std::to_string(calculateAverage(m_gpuRenderTimes)) + ",\n";
    report += "    \"AverageCpuUsagePercent\": " + std::to_string(calculateAverage(m_cpuUsages)) + "\n";
    report += "  }\n";
    report += "}";

    return report;
}

void MediaTelemetry::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_decodeLatencies.clear();
    m_importLatencies.clear();
    m_presentationLatencies.clear();
    m_cpuUsages.clear();
    m_gpuRenderTimes.clear();
    m_fallbackReason.clear();
}

} // namespace luma::media
