#include <render/common/IRenderBackendFactory.hpp>
#include <render/common/BackendRegistry.hpp>
#include "MockRenderer.hpp"

namespace luma::render::mock {

class MockBackendFactory : public IRenderBackendFactory {
public:
    std::string getName() const override { return "Mock"; }
    int getPriority() const override { return 0; } // Lowest priority
    bool isSupported() const override { return true; } // Always supported

    std::unique_ptr<IRenderBackend> create() override {
        return std::make_unique<MockBackend>();
    }
};

void registerMockBackend() {
    static bool registered = false;
    if (!registered) {
        BackendRegistry::get().registerFactory(std::make_unique<MockBackendFactory>());
        registered = true;
    }
}

} // namespace luma::render::mock
