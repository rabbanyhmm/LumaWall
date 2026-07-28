find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED IMPORTED_TARGET wayland-client)

add_library(lumawall_wayland
    wayland/core/DisplayConnection.cpp
    wayland/core/EventLoop.cpp
    wayland/core/Registry.cpp
    wayland/core/OutputManager.cpp
    wayland/protocols/LayerShell.cpp
    wayland/WaylandWallpaperSurface.cpp
)
target_include_directories(lumawall_wayland PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner REQUIRED)

function(add_wayland_protocol target protocol_xml)
    get_filename_component(protocol_name ${protocol_xml} NAME_WE)
    set(header_out ${CMAKE_CURRENT_BINARY_DIR}/${protocol_name}-client-protocol.h)
    set(code_out ${CMAKE_CURRENT_BINARY_DIR}/${protocol_name}-protocol.c)

    add_custom_command(
        OUTPUT ${header_out}
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header ${protocol_xml} ${header_out}
        DEPENDS ${protocol_xml}
    )

    add_custom_command(
        OUTPUT ${code_out}
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code ${protocol_xml} ${code_out}
        DEPENDS ${protocol_xml}
    )

    target_sources(${target} PRIVATE ${code_out} ${header_out})
    target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
endfunction()

add_wayland_protocol(lumawall_wayland ${CMAKE_SOURCE_DIR}/resources/protocols/xdg-shell.xml)
add_wayland_protocol(lumawall_wayland ${CMAKE_SOURCE_DIR}/resources/protocols/wlr-layer-shell-unstable-v1.xml)

target_link_libraries(lumawall_wayland PUBLIC PkgConfig::WAYLAND spdlog::spdlog)

add_subdirectory(x11)

add_library(lumawall_platform STATIC PlatformFactory.cpp)
target_include_directories(lumawall_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(lumawall_platform PUBLIC lumawall_wayland lumawall_x11 lumawall_core)
