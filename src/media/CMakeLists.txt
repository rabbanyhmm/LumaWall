file(GLOB_RECURSE MEDIA_SOURCES 
    "common/*.cpp"
    "capabilities/*.cpp"
    "image/*.cpp"
    "scheduler/*.cpp"
    "ffmpeg/*.cpp"
    "hardware/*.cpp"
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libswscale)

add_library(lumawall_media STATIC
    ${MEDIA_SOURCES}
)

target_include_directories(lumawall_media PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(lumawall_media PUBLIC
    lumawall_core
    lumawall_render
    PkgConfig::FFMPEG
)
