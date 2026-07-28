add_executable(lumawall 
    main.cpp 
    DBusServer.cpp
    monitor/MonitorContext.cpp
    monitor/MonitorContext.hpp
    monitor/MonitorManager.cpp
    monitor/MonitorManager.hpp
    ../platform/gnome/GnomeIntegration.cpp
    ../platform/gnome/GnomeIntegration.hpp
)
set_target_properties(lumawall PROPERTIES AUTOMOC ON)
target_link_libraries(lumawall PRIVATE lumawall_core lumawall_platform lumawall_render lumawall_media lumawall_library lumawall_warnings Qt6::Core Qt6::Gui Qt6::DBus)
