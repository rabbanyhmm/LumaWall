cmake_minimum_required(VERSION 3.25)
project(LumaWall VERSION 0.1.0 LANGUAGES CXX C)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(LUMA_BUILD_TESTS "Build GoogleTest unit tests" OFF)
option(LUMA_ENABLE_CLANG_TIDY "Enable clang-tidy analysis during compilation" OFF)

# Find Dependencies
find_package(Qt6 COMPONENTS Core Gui Widgets DBus REQUIRED)
find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil libswscale)
find_package(SQLite3 REQUIRED)

# We will use spdlog for logging. Let's assume it's available via system or will be provided.
# find_package(spdlog REQUIRED)

# Common compiler warnings
add_library(lumawall_warnings INTERFACE)
target_compile_options(lumawall_warnings INTERFACE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2
    >
)

# Enable clang-tidy if requested
if(LUMA_ENABLE_CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*,-fuchsia-*,-google-readability-todo,-llvm-header-guard,-cppcoreguidelines-init-variables")
endif()

# Tests are always built (src/CMakeLists.txt always includes src/tests).
# enable_testing() MUST be called at root scope BEFORE add_subdirectory
# so that CTest generates a root-level CTestTestfile.cmake and discovers
# all gtest_discover_tests() calls in subdirectories.
enable_testing()
include(CTest)

# Include subdirectories
add_subdirectory(src)
