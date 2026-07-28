#pragma once
#include <string>

namespace luma::media {

enum class MediaType {
    Video,
    Image,
    Audio,
    Unknown
};

class IMediaSource {
public:
    virtual ~IMediaSource() = default;

    virtual std::string getURI() const = 0;
    virtual MediaType getType() const = 0;
    virtual double getDuration() const = 0;
};

} // namespace luma::media
