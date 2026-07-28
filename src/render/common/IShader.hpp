#pragma once
#include <string_view>
#include <vector>
#include <cstdint>

namespace luma::render {

enum class ShaderStage {
    Vertex,
    Fragment,
    Compute
};

class IShader {
public:
    virtual ~IShader() = default;

    virtual ShaderStage getStage() const = 0;
};

} // namespace luma::render
