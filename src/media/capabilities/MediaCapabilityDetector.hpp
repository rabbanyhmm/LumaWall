#pragma once
#include <media/capabilities/MediaCapabilities.hpp>

namespace luma::media {

class MediaCapabilityDetector {
public:
    static MediaCapabilities detect();
};

} // namespace luma::media
