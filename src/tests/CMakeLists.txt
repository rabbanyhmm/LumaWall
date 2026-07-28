pkg_check_modules(WAYLAND_SERVER REQUIRED IMPORTED_TARGET wayland-server)

add_executable(lumawall_wayland_tests
    WaylandMockTests.cpp
)

target_include_directories(lumawall_wayland_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../platform)

target_link_libraries(lumawall_wayland_tests PRIVATE
    gtest_main
    lumawall_wayland
    lumawall_core
    PkgConfig::WAYLAND_SERVER
)

include(GoogleTest)
gtest_discover_tests(lumawall_wayland_tests)

add_executable(lumawall_x11_tests X11Tests.cpp)
target_include_directories(lumawall_x11_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../platform)
target_link_libraries(lumawall_x11_tests PRIVATE gtest_main lumawall_x11 lumawall_platform lumawall_core)
gtest_discover_tests(lumawall_x11_tests)

add_executable(lumawall_render_tests RenderTests.cpp)
target_include_directories(lumawall_render_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(lumawall_render_tests PRIVATE gtest_main lumawall_render lumawall_core)
add_executable(lumawall_vulkan_tests VulkanTests.cpp)
target_include_directories(lumawall_vulkan_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(lumawall_vulkan_tests PRIVATE gtest_main lumawall_render lumawall_core Vulkan::Vulkan)
gtest_discover_tests(lumawall_vulkan_tests)

add_executable(lumawall_media_tests media_pipeline_test.cpp)
target_include_directories(lumawall_media_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(lumawall_media_tests PRIVATE gtest_main lumawall_media lumawall_core)
gtest_discover_tests(lumawall_media_tests)

add_executable(lumawall_monitor_tests 
    monitor_orchestrator_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/monitor/MonitorManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/monitor/MonitorContext.cpp
)
target_include_directories(lumawall_monitor_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../app)
target_link_libraries(lumawall_monitor_tests PRIVATE gtest_main lumawall_core lumawall_media lumawall_render lumawall_library Vulkan::Vulkan)
gtest_discover_tests(lumawall_monitor_tests)
