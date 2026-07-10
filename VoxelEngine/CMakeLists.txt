cmake_minimum_required(VERSION 3.20)

project(VoxelEngine 
    VERSION 0.1.0
    LANGUAGES CXX)

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_library(${PROJECT_NAME} STATIC ${ENGINE_SOURCES})

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/include/pch.h>"
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

include(cmake/dependencies.cmake)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(assimp PRIVATE -Wno-unknown-pragmas -Wno-implicit-fallthrough)
    target_compile_options(${PROJECT_NAME} PRIVATE -static-libgcc)
endif()
