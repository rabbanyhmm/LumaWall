#pragma once

#include <memory>
#include <string>
#include <expected>

namespace luma::core {

enum class EngineError {
    AlreadyInitialized,
    NotInitialized,
    PlatformError,
    RenderError
};

class Engine {
public:
    Engine();
    ~Engine();

    // Delete copy and move semantics for singleton-like usage or strict lifecycle
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    std::expected<void, EngineError> init();
    void shutdown();
    void pause();
    void resume();

    [[nodiscard]] bool isRunning() const noexcept { return m_isRunning; }

private:
    bool m_isRunning{false};
};

} // namespace luma::core
