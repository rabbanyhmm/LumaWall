#include "BackendRegistry.hpp"
#include "IRenderBackend.hpp"
#include <core/Logging.hpp>
#include <algorithm>

namespace luma::render {

BackendRegistry& BackendRegistry::get() {
    static BackendRegistry instance;
    return instance;
}

void BackendRegistry::registerFactory(std::unique_ptr<IRenderBackendFactory> factory) {
    if (factory) {
        spdlog::debug("[RENDER] Registered backend factory: {}", factory->getName());
        m_factories.push_back(std::move(factory));
    }
}

std::unique_ptr<IRenderBackend> BackendRegistry::createBestAvailableBackend() const {
    std::vector<IRenderBackendFactory*> supportedFactories;
    
    for (const auto& factory : m_factories) {
        if (factory->isSupported()) {
            supportedFactories.push_back(factory.get());
        }
    }

    if (supportedFactories.empty()) {
        spdlog::error("[RENDER] No supported rendering backends found!");
        return nullptr;
    }

    std::sort(supportedFactories.begin(), supportedFactories.end(), 
        [](IRenderBackendFactory* a, IRenderBackendFactory* b) {
            return a->getPriority() > b->getPriority();
        });

    auto* best = supportedFactories.front();
    spdlog::info("[RENDER] Selected best available backend: {}", best->getName());
    
    return best->create();
}

std::vector<std::string> BackendRegistry::getAvailableBackends() const {
    std::vector<std::string> names;
    for (const auto& factory : m_factories) {
        if (factory->isSupported()) {
            names.push_back(factory->getName());
        }
    }
    return names;
}

} // namespace luma::render
