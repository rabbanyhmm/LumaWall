#pragma once
#include "IRenderBackendFactory.hpp"
#include <vector>
#include <memory>
#include <optional>

namespace luma::render {

class BackendRegistry {
public:
    static BackendRegistry& get();

    void registerFactory(std::unique_ptr<IRenderBackendFactory> factory);
    std::unique_ptr<IRenderBackend> createBestAvailableBackend() const;
    std::vector<std::string> getAvailableBackends() const;

private:
    BackendRegistry() = default;
    std::vector<std::unique_ptr<IRenderBackendFactory>> m_factories;
};

} // namespace luma::render
