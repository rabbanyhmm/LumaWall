#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <vector>
#include <cstdlib>
#include <string>

namespace luma::core {

class Logging {
public:
    static void init() {
        try {
            const char* home = std::getenv("HOME");
            std::string logDir = home ? std::string(home) + "/.local/share/lumawall/logs/latest.log" : "/tmp/lumawall/latest.log";

            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::debug);
            console_sink->set_pattern("[%^%l%$] %v");

            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logDir, 1048576 * 5, 3);
            file_sink->set_level(spdlog::level::trace);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

            std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
            auto logger = std::make_shared<spdlog::logger>("lumawall", sinks.begin(), sinks.end());
            
            spdlog::register_logger(logger);
            spdlog::set_default_logger(logger);
            spdlog::set_level(spdlog::level::debug);
            spdlog::flush_on(spdlog::level::info);
        } catch (const spdlog::spdlog_ex& ex) {
            // fallback
        }
    }
};

} // namespace luma::core
