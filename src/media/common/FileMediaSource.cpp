#include "FileMediaSource.hpp"

namespace luma::media {

FileMediaSource::FileMediaSource(const std::string& filepath, MediaType type, double duration)
    : m_filepath(filepath), m_type(type), m_duration(duration) {
}

} // namespace luma::media
