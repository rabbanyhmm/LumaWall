#include "Engine.hpp"
#include <iostream> // Replace with spdlog later

namespace luma::core {

Engine::Engine() = default;

Engine::~Engine() {
    shutdown();
}

std::expected<void, EngineError> Engine::init() {
    if (m_isRunning) {
        return std::unexpected(EngineError::AlreadyInitialized);
    }
    
    // Platform and Render initialization will go here
    m_isRunning = true;
    std::cout << "LumaWall Engine Initialized.\n";
    return {};
}

void Engine::shutdown() {
    if (m_isRunning) {
        // Shutdown subsystems
        std::cout << "LumaWall Engine Shutting down.\n";
        m_isRunning = false;
    }
}

void Engine::pause() {
    if (m_isRunning) {
        std::cout << "LumaWall Engine Paused.\n";
    }
}

void Engine::resume() {
    if (m_isRunning) {
        std::cout << "LumaWall Engine Resumed.\n";
    }
}

} // namespace luma::core
