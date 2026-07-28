include(FetchContent)

# Find Vulkan 1.3
find_package(Vulkan REQUIRED)
if (Vulkan_FOUND)
    message(STATUS "Vulkan Found: ${Vulkan_VERSION}")
endif()

# Fetch VMA
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        v3.1.0 # Suitable for Vulkan 1.3
)
FetchContent_MakeAvailable(vma)

# Fetch stb_image
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

# Collect Vulkan Sources
file(GLOB_RECURSE VULKAN_SOURCES "vulkan/*.cpp")

add_library(lumawall_render STATIC
    common/BackendRegistry.cpp
    common/RenderGraph.cpp
    common/RenderScheduler.cpp
    common/RenderResourceManager.cpp
    mock/MockRenderer.cpp
    ${VULKAN_SOURCES}
)

target_include_directories(lumawall_render PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Link Vulkan and VMA
target_link_libraries(lumawall_render PUBLIC
    lumawall_core
    lumawall_platform
    lumawall_media
    Vulkan::Vulkan
)

# Note: VMA is a header-only library, so we just include its directory
target_include_directories(lumawall_render SYSTEM PUBLIC
    ${vma_SOURCE_DIR}/include
    ${stb_SOURCE_DIR}
)

# Shader Compilation
find_program(GLSLC glslangValidator)
if(GLSLC)
    message(STATUS "Found glslangValidator: ${GLSLC}")
    
    set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
    set(SHADER_SOURCES 
        ${SHADER_DIR}/fullscreen.vert
        ${SHADER_DIR}/textured.frag
    )
    
    foreach(SHADER_SRC ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER_SRC} NAME)
        set(SPV_OUT ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv)
        
        add_custom_command(
            OUTPUT ${SPV_OUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders
            COMMAND ${GLSLC} -V ${SHADER_SRC} -o ${SPV_OUT}
            DEPENDS ${SHADER_SRC}
            COMMENT "Compiling shader ${SHADER_NAME} to SPIR-V"
        )
        
        list(APPEND SPV_TARGETS ${SPV_OUT})
    endforeach()
    
    add_custom_target(compile_shaders ALL DEPENDS ${SPV_TARGETS})
    add_dependencies(lumawall_render compile_shaders)
else()
    message(WARNING "glslangValidator not found! Shaders will not be compiled automatically.")
endif()
