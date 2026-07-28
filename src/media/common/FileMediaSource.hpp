#pragma once
#include <media/common/IMediaSource.hpp>

namespace luma::media {

class FileMediaSource : public IMediaSource {
public:
    FileMediaSource(const std::string& filepath, MediaType type, double duration = 0.0);
    ~FileMediaSource() override = default;

    std::string getURI() const override { return m_filepath; }
    MediaType getType() const override { return m_type; }
    double getDuration() const override { return m_duration; }

private:
    std::string m_filepath;
    MediaType m_type;
    double m_duration;
};

} // namespace luma::media
