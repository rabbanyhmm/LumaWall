include(FetchContent)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.12.0
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)
enable_testing()

add_subdirectory(core)
add_subdirectory(platform)
add_subdirectory(media)
add_subdirectory(render)
add_subdirectory(library)
add_subdirectory(ui)
add_subdirectory(app)
add_subdirectory(tests)

add_custom_target(install_gnome_extension
    COMMAND npm install --prefix ${CMAKE_CURRENT_SOURCE_DIR}/platform/gnome/extension
    COMMAND npm run build --prefix ${CMAKE_CURRENT_SOURCE_DIR}/platform/gnome/extension
    COMMAND ${CMAKE_COMMAND} -E make_directory $ENV{HOME}/.local/share/gnome-shell/extensions/lumawall-integration@lumawall.org
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/platform/gnome/extension/metadata.json $ENV{HOME}/.local/share/gnome-shell/extensions/lumawall-integration@lumawall.org/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/platform/gnome/extension/extension.js $ENV{HOME}/.local/share/gnome-shell/extensions/lumawall-integration@lumawall.org/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/platform/gnome/extension/stylesheet.css $ENV{HOME}/.local/share/gnome-shell/extensions/lumawall-integration@lumawall.org/
    COMMENT "Installing GNOME integration extension to ~/.local/share/gnome-shell/extensions/"
)

