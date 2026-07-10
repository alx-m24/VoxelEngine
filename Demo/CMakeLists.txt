cmake_minimum_required(VERSION 3.20)

project(Demo LANGUAGES CXX)

file(GLOB_RECURSE DEMO_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable(${PROJECT_NAME}
    ${DEMO_SOURCES}
)

target_compile_features(${PROJECT_NAME}
    PRIVATE
        cxx_std_23
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        VoxelEngine
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# --- Windows runtime (MinGW only) ---
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    foreach(dll libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${dll}
            OUTPUT_VARIABLE DLL_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(EXISTS ${DLL_PATH})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL_PATH}"
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
            )
        endif()
    endforeach()
endif()

