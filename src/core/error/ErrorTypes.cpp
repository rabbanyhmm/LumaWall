#include "ErrorTypes.hpp"
#include <core/Logging.hpp>

namespace luma::core::error {

ErrorHandler& ErrorHandler::instance() {
    static ErrorHandler s_instance;
    return s_instance;
}

void ErrorHandler::registerCallback(std::function<void(const ErrorDetails&)> callback) {
    m_callbacks.push_back(callback);
}

void ErrorHandler::reportError(ErrorCode code, const std::string& message, const std::string& subsystem) {
    spdlog::error("[{}] Error {}: {}", subsystem, static_cast<int>(code), message);
    
    ErrorDetails details{code, message, subsystem};
    for (auto& cb : m_callbacks) {
        if (cb) {
            cb(details);
        }
    }
}

} // namespace luma::core::error
