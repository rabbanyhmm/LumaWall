#pragma once

namespace luma::platform {

enum class DisplayMode {
    Independent,
    Mirror,
    Span
};

enum class ScalingMode {
    Fill,
    Fit,
    Stretch,
    Center,
    Tile,
    Crop
};

} // namespace luma::platform
