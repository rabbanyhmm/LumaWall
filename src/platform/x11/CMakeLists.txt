pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)
pkg_check_modules(XCB_RANDR REQUIRED IMPORTED_TARGET xcb-randr)
pkg_check_modules(XCB_EWMH REQUIRED IMPORTED_TARGET xcb-ewmh)
pkg_check_modules(XCB_ICCCM REQUIRED IMPORTED_TARGET xcb-icccm)

add_library(lumawall_x11 STATIC
    core/Connection.cpp
    core/ScreenManager.cpp
    core/WindowManager.cpp
    core/AtomCache.cpp
    core/EventLoop.cpp
    randr/OutputManager.cpp
    wallpaper/DesktopWindow.cpp
    wallpaper/X11WallpaperSurface.cpp
    X11PlatformBackend.cpp
)

target_include_directories(lumawall_x11 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(lumawall_x11 PUBLIC 
    PkgConfig::XCB 
    PkgConfig::XCB_RANDR 
    PkgConfig::XCB_EWMH 
    PkgConfig::XCB_ICCCM
    lumawall_core
)
