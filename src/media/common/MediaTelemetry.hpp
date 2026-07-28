#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <map>

namespace luma::media {

struct FrameMetrics {
    double decodeLatencyMs{0.0};
    double importLatencyMs{0.0};
    double presentationLatencyMs{0.0}; // from decode request to screen
};

class MediaTelemetry {
public:
    static MediaTelemetry& getInstance() {
        static MediaTelemetry instance;
        return instance;
    }

    void recordDecodeLatency(double ms);
    void recordImportLatency(double ms);
    void recordPresentationLatency(double ms);
    void recordCpuUsage(double percent);
    void recordGpuRenderTime(double ms);

    void setPlaybackInfo(const std::string& codec, const std::string& resolution, 
                         const std::string& pixelFormat, const std::string& decoderBackend,
                         bool hardwareAccelerated, bool zeroCopy, const std::string& uploadPath);
    void setFallbackReason(const std::string& reason);

    std::string generateCompatibilityReport() const;
    void reset();

private:
    MediaTelemetry() = default;
    
    mutable std::mutex m_mutex;
    
    // Playback Metadata
    std::string m_codec;
    std::string m_resolution;
    std::string m_pixelFormat;
    std::string m_decoderBackend;
    bool m_hardwareAccelerated{false};
    bool m_zeroCopy{false};
    std::string m_uploadPath;
    std::string m_fallbackReason;

    // Metrics
    std::vector<double> m_decodeLatencies;
    std::vector<double> m_importLatencies;
    std::vector<double> m_presentationLatencies;
    std::vector<double> m_cpuUsages;
    std::vector<double> m_gpuRenderTimes;

    double calculateAverage(const std::vector<double>& data) const;
};

} // namespace luma::media
