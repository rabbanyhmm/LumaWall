set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

add_executable(lumawall-ui
    main.cpp
    MainWindow.cpp
    WallpaperBrowser.cpp
    PlaylistEditor.cpp
    PerformanceDashboard.cpp
    TrayIcon.cpp
)

target_link_libraries(lumawall-ui PRIVATE
    lumawall_library
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
    spdlog::spdlog
)
