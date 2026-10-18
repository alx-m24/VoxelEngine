cmake_minimum_required(VERSION 3.21)
project(VoxelEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Executable and compiled shaders both end up in <build>/ (any config).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
foreach(cfg IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER ${cfg} CFG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} ${CMAKE_BINARY_DIR})
endforeach()

# ---------------------------------------------------------------------------
# Dependencies (all fetched at configure time)
#   Vulkan-Headers : vulkan.h / vulkan.hpp. The loader itself is loaded at
#                    runtime (vulkan-1.dll / libvulkan.so.1), so nothing to link.
#   VMA            : Vulkan Memory Allocator (header-only)
#   GLFW, GLM      : window + math
# ---------------------------------------------------------------------------
include(FetchContent)

set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

FetchContent_Declare(vulkan_headers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG        v1.3.296
    GIT_SHALLOW    TRUE)
FetchContent_Declare(vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        v3.3.0
    GIT_SHALLOW    TRUE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(vulkan_headers vma glfw glm)

# ---------------------------------------------------------------------------
# Application
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME} src/main.cpp src/vma.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Headers glfw glm::glm)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${vma_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VK_NO_PROTOTYPES                       # functions come from the dynamic dispatcher
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1    # structs become aggregates -> C++20 designated initializers
    VMA_STATIC_VULKAN_FUNCTIONS=0
    VMA_DYNAMIC_VULKAN_FUNCTIONS=1
    VMA_VULKAN_VERSION=1003000
    GLM_FORCE_DEPTH_ZERO_TO_ONE            # Vulkan clip space z in [0,1]
    GLM_FORCE_RADIANS)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
    set_source_files_properties(src/vma.cpp PROPERTIES COMPILE_OPTIONS /w)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
    set_source_files_properties(src/vma.cpp PROPERTIES COMPILE_OPTIONS -w)
endif()

# ---------------------------------------------------------------------------
# Shader compilation: shaders/*.{vert,frag,comp,...} -> bin/shaders/<name>.spv
# Incremental: each .spv has its own custom command that depends on its source
# (and, with Ninja/Makefiles, on every #include'd file via a compiler depfile).
# New shader files are picked up on the next build (CONFIGURE_DEPENDS).
# ---------------------------------------------------------------------------
if(DEFINED ENV{VULKAN_SDK})
    set(_sdk_hints "$ENV{VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/Bin")
endif()
find_program(GLSLC_EXECUTABLE   glslc            HINTS ${_sdk_hints})
find_program(GLSLANG_EXECUTABLE glslangValidator HINTS ${_sdk_hints})
if(NOT GLSLC_EXECUTABLE AND NOT GLSLANG_EXECUTABLE)
    message(FATAL_ERROR
        "No shader compiler found. Install the Vulkan SDK (https://vulkan.lunarg.com), "
        "or 'apt install glslc' / 'brew install shaderc', and re-run CMake.")
endif()

file(GLOB SHADER_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/shaders/*.vert ${CMAKE_SOURCE_DIR}/shaders/*.frag
    ${CMAKE_SOURCE_DIR}/shaders/*.comp ${CMAKE_SOURCE_DIR}/shaders/*.geom
    ${CMAKE_SOURCE_DIR}/shaders/*.tesc ${CMAKE_SOURCE_DIR}/shaders/*.tese)

set(SHADER_OUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)
set(SPIRV_FILES "")

foreach(src IN LISTS SHADER_SOURCES)
    get_filename_component(fname ${src} NAME)
    set(spv ${SHADER_OUT_DIR}/${fname}.spv)

    if(GLSLC_EXECUTABLE)
        set(compile_cmd ${GLSLC_EXECUTABLE} --target-env=vulkan1.3
            $<IF:$<CONFIG:Debug>,-g,-O> -MD -MF ${spv}.d ${src} -o ${spv})
    else()
        set(compile_cmd ${GLSLANG_EXECUTABLE} -V --target-env vulkan1.3 ${src} -o ${spv})
    endif()

    set(depfile_arg "")
    if(GLSLC_EXECUTABLE AND CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
        set(depfile_arg DEPFILE ${spv}.d)
    endif()

    add_custom_command(
        OUTPUT  ${spv}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
        COMMAND ${compile_cmd}
        DEPENDS ${src}
        ${depfile_arg}
        COMMENT "Compiling shader ${fname}"
        VERBATIM)
    list(APPEND SPIRV_FILES ${spv})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPIRV_FILES})
add_dependencies(${PROJECT_NAME} shaders)

# The program loads <SHADER_DIR>/<name>.spv
target_compile_definitions(${PROJECT_NAME} PRIVATE SHADER_DIR="${SHADER_OUT_DIR}")
