#pragma once
#include <string>
#include <functional>
#include <vector>

namespace luma::core::error {

enum class ErrorCode {
    Success = 0,
    
    // Media Errors (1000+)
    MediaFileNotFound = 1001,
    MediaUnsupportedCodec = 1002,
    MediaDecodeFailed = 1003,
    
    // Renderer Errors (2000+)
    VulkanDeviceLost = 2001,
    VulkanOutOfMemory = 2002,
    VulkanSurfaceLost = 2003,
    
    // System Errors (3000+)
    DatabaseCorrupted = 3001,
    PluginLoadFailed = 3002
};

struct ErrorDetails {
    ErrorCode code;
    std::string message;
    std::string subsystem;
};

class ErrorHandler {
public:
    static ErrorHandler& instance();

    // Register a callback to receive errors (e.g., the DBusServer can listen here)
    void registerCallback(std::function<void(const ErrorDetails&)> callback);

    // Report an error
    void reportError(ErrorCode code, const std::string& message, const std::string& subsystem = "Core");

private:
    ErrorHandler() = default;
    ~ErrorHandler() = default;

    std::vector<std::function<void(const ErrorDetails&)>> m_callbacks;
};

} // namespace luma::core::error
