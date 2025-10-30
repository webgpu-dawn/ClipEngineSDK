cmake_minimum_required(VERSION 3.13)
project(example)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CLIPENGINE_SDK_BASE "${CMAKE_CURRENT_SOURCE_DIR}/deps")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/ffmpeg_x64-windows/share/ffmpeg)
list(APPEND CMAKE_PREFIX_PATH
    "${CLIPENGINE_SDK_BASE}/ClipEngineSDK-Debug"
    "${CLIPENGINE_SDK_BASE}/ClipEngineSDK-Release"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/ffmpeg_x64-windows"
)

# The SDK will be selected at build time based on the configuration
find_package(ClipEngine REQUIRED)

# Find FFmpeg using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
)

# Note: GLFW and ImGui are now provided by ClipEngine::clipengine_debug
# No need to build them again in example

message(STATUS "Building video example as standalone test project")
message(STATUS "  ClipEngine version: ${ClipEngine_VERSION}")
message(STATUS "  ClipEngine include: ${ClipEngine_INCLUDE_DIRS}")
message(STATUS "  ClipEngine libraries: ${ClipEngine_LIBRARIES}")
message(STATUS "  ClipEngine shaders: ${ClipEngine_SHADER_DIR}")

add_executable(
    ${PROJECT_NAME}
    main.cpp
    Decoder.cpp
    Application.cpp
)

# Link against ClipEngine SDK and other dependencies
# ClipEngine DLLs will be automatically copied when linking ClipEngine::clipengine
target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    ClipEngine::clipengine
    ClipEngine::clipengine_debug  # Debug window support
    PkgConfig::FFMPEG
    d3d11
    dxgi
    d3dcompiler
)

# Copy ClipEngine DLLs to output directory
# When building from source, copy from deps directory
# When using SDK, copy from SDK bin directory
if(TARGET clipengine)
    # Building from source - copy from deps directory
    set(CLIPENGINE_DLL_SOURCE "${CMAKE_SOURCE_DIR}/deps/dawn/bin")
    set(CLIPENGINE_EXTRA_DLL_SOURCE "${CMAKE_SOURCE_DIR}/deps/extra")
else()
    # Using SDK - copy from SDK bin directory
    set(CLIPENGINE_DLL_SOURCE "${ClipEngine_BIN_DIR}")
    set(CLIPENGINE_EXTRA_DLL_SOURCE "${ClipEngine_BIN_DIR}")
endif()

# Copy ClipEngine required DLLs
set(CLIPENGINE_DLLS
    webgpu_dawn.dll
    d3dcompiler_47.dll
    vulkan-1.dll
)

foreach(DLL ${CLIPENGINE_DLLS})
    if(EXISTS "${CLIPENGINE_DLL_SOURCE}/${DLL}")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CLIPENGINE_DLL_SOURCE}/${DLL}"
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying ClipEngine DLL: ${DLL}"
        )
    elseif(EXISTS "${CLIPENGINE_EXTRA_DLL_SOURCE}/${DLL}")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CLIPENGINE_EXTRA_DLL_SOURCE}/${DLL}"
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying ClipEngine DLL: ${DLL}"
        )
    endif()
endforeach()

# Copy shader files to output directory
if(TARGET clipengine)
    # Building from source - copy shaders directly from source directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/src/clipengine/shaders"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
        COMMENT "Copying ClipEngine shader files from source"
    )
elseif(ClipEngine_SHADER_DIR AND EXISTS "${ClipEngine_SHADER_DIR}")
    # Using SDK - copy from SDK shader directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ClipEngine_SHADER_DIR}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
        COMMENT "Copying ClipEngine shader files from SDK"
    )
endif()

set_directory_properties(PROPERTIES VS_STARTUP_PROJECT ${PROJECT_NAME})
