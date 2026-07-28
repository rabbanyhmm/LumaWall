add_library(lumawall_core 
    Engine.cpp 
    Engine.hpp
    plugin/PluginManager.cpp
    plugin/PluginManager.hpp
    plugin/IPlugin.hpp
    SettingsManager.cpp
    SettingsManager.hpp
    task/TaskManager.cpp
    task/TaskManager.hpp
    system/CapabilityManager.cpp
    system/CapabilityManager.hpp
    error/ErrorTypes.cpp
    error/ErrorTypes.hpp
)
set_target_properties(lumawall_core PROPERTIES AUTOMOC ON)
target_include_directories(lumawall_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(lumawall_core PUBLIC lumawall_warnings spdlog::spdlog Qt6::Core)
