#pragma once
#include <platform/common/IPlatformBackend.hpp>
#include <memory>

namespace luma::platform {

class PlatformFactory {
public:
    static std::unique_ptr<IPlatformBackend> create();
};

} // namespace luma::platform
