include(FetchContent)

# Fetch SQLiteCpp
FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG        3.3.1
)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "Run cpplint.py on source files" FORCE)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "Run cppcheck on source files" FORCE)
set(SQLITECPP_BUILD_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
set(SQLITECPP_BUILD_TESTS OFF CACHE BOOL "Build and run tests" FORCE)
set(SQLITECPP_INTERNAL_SQLITE ON CACHE BOOL "Use internal SQLite3 build" FORCE)

FetchContent_MakeAvailable(SQLiteCpp)

file(GLOB_RECURSE LIBRARY_SOURCES "*.cpp")

add_library(lumawall_library STATIC
    ${LIBRARY_SOURCES}
)

target_include_directories(lumawall_library PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(lumawall_library PUBLIC
    SQLiteCpp
    lumawall_core
    spdlog::spdlog
    Qt6::Core
    Qt6::Gui
    PkgConfig::FFMPEG
)
