cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0111 NEW)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    set(CMAKE_TOOLCHAIN_FILE "D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")
elseif(APPLE)
    message(STATUS "Building on MacOS")
elseif(UNIX)
    message(STATUS "Building on Linux")
endif()

project(clipengine)

# 设置 SDK 安装路径
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/output/ClipEngineSDK-$<CONFIG>" CACHE PATH "Default install path" FORCE)
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_PREFIX_PATH
    ${CMAKE_SOURCE_DIR}/deps/dawn/lib/cmake
    ${CMAKE_SOURCE_DIR}/deps/spdlog/shard/spdlog
)
find_package(Dawn REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Find FFmpeg libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
)

# Optional: ImGui for debug module (using FetchContent)
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

include_directories(
    deps/dawn/include
    ${PROJECT_SOURCE_DIR}/src
)

# ============================================================================
# ClipEngine Library - Professional Video Editing Engine Core
# ============================================================================

add_library(clipengine STATIC
    # Core
    src/clipengine/core/RenderDevice.cpp
    src/clipengine/core/CompositionEngine.cpp
    src/clipengine/core/OffscreenRenderer.cpp

    # Layers
    src/clipengine/layers/TextureRenderer.cpp
    src/clipengine/layers/VideoRenderer.cpp

    # Effects
    src/clipengine/effects/Filter.cpp
    src/clipengine/effects/FilterChain.cpp
    src/clipengine/effects/ShaderEffect.cpp
    src/clipengine/effects/EffectLoader.cpp

    # Shaders
    src/clipengine/shaders/ShaderLibrary.cpp
    src/clipengine/shaders/ShaderConfig.cpp

    # Utilities
    src/clipengine/utils/RenderUtils.cpp
    src/clipengine/utils/RuntimeInspector.cpp
    src/clipengine/utils/GPUTimer.cpp

    # Export
    src/clipengine/export/VideoExporter.cpp

    # Note: Debug module (DebugWindow) is provided as source code in debug/
    # Applications can optionally include and compile it with ImGui/GLFW support
)

# Set public headers for installation
set(CLIPENGINE_PUBLIC_HEADERS
    # Main header
    src/clipengine/clipengine.h

    # Core
    src/clipengine/core/CompositionLayer.h
    src/clipengine/core/CompositionEngine.h
    src/clipengine/core/RenderDevice.h
    src/clipengine/core/OffscreenRenderer.h
    src/clipengine/core/InputState.h

    # Layers
    src/clipengine/layers/TextureRenderer.h
    src/clipengine/layers/VideoRenderer.h

    # Effects
    src/clipengine/effects/Filter.h
    src/clipengine/effects/FilterChain.h
    src/clipengine/effects/ShaderEffect.h
    src/clipengine/effects/EffectLoader.h

    # Shaders
    src/clipengine/shaders/ShaderLibrary.h
    src/clipengine/shaders/ShaderConfig.h

    # Export
    src/clipengine/export/VideoExporter.h

    # Note: Debug headers (DebugWindow) are NOT exported
    # They are optional and available as source code only
)

# Configure library properties
target_include_directories(clipengine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/deps/dawn/include>
        $<INSTALL_INTERFACE:include>
        $<INSTALL_INTERFACE:include/dawn>
)

target_compile_definitions(clipengine
    PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
)

target_link_libraries(clipengine
    PUBLIC
        dawn::webgpu_dawn
    PRIVATE
        spdlog::spdlog_header_only
        fmt::fmt
        glm::glm
        PkgConfig::FFMPEG
)

# Alias for consistent naming
add_library(ClipEngine::clipengine ALIAS clipengine)

# ============================================================================
# ClipEngine Debug Library (Optional)
# ============================================================================
# Separate library for debug features to avoid forcing ImGui dependency on all users

# Find GLFW from example deps (or system)
set(GLFW_SOURCE_DIR "${CMAKE_SOURCE_DIR}/example/deps/glfw")
if(EXISTS "${GLFW_SOURCE_DIR}/CMakeLists.txt")
    add_subdirectory(${GLFW_SOURCE_DIR} ${CMAKE_BINARY_DIR}/glfw EXCLUDE_FROM_ALL)
else()
    find_package(glfw3 REQUIRED)
endif()

# Create ImGui core library
add_library(imgui_core STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui_core PUBLIC ${imgui_SOURCE_DIR})

# Create ImGui backends library
add_library(imgui_backends STATIC
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_wgpu.cpp
)
target_include_directories(imgui_backends PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_compile_definitions(imgui_backends PUBLIC
    IMGUI_IMPL_WEBGPU_BACKEND_DAWN
)
target_link_libraries(imgui_backends PUBLIC
    imgui_core
    glfw
    dawn::webgpu_dawn
)

# ClipEngine Debug library
add_library(clipengine_debug STATIC
    src/clipengine/debug/DebugWindow.cpp
)

target_include_directories(clipengine_debug
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(clipengine_debug
    PUBLIC
        clipengine
        imgui_backends
        glfw
)

# Alias for consistent naming
add_library(ClipEngine::clipengine_debug ALIAS clipengine_debug)

# ============================================================================
# Installation Rules
# ============================================================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install libraries (without EXPORT to avoid exporting PRIVATE dependencies)
install(TARGETS clipengine clipengine_debug imgui_core imgui_backends
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers with directory structure
# Exclude CeLogger.h to avoid exposing spdlog dependency
install(DIRECTORY src/clipengine/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/clipengine
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "CeLogger.h" EXCLUDE
)

# Install Dawn headers (required dependency)
install(DIRECTORY deps/dawn/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dawn
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install shaders
install(DIRECTORY src/clipengine/shaders/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/clipengine/shaders
    FILES_MATCHING PATTERN "*.wgsl"
)

# Install Dawn DLLs and libraries
install(FILES deps/dawn/bin/webgpu_dawn.dll
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
# Install Dawn lib files to both lib/ and lib/dawn/
install(FILES deps/dawn/lib/webgpu_dawn.lib
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY deps/dawn/lib/
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/dawn
    FILES_MATCHING PATTERN "*.lib" PATTERN "*.a"
)

# Install extra runtime dependencies (D3D compiler, Vulkan)
# These are required for Dawn/WebGPU to work on Windows
set(EXTRA_RUNTIME_DLLS
    deps/extra/d3dcompiler_47.dll
    deps/extra/vulkan-1.dll
)

foreach(DLL ${EXTRA_RUNTIME_DLLS})
    if(EXISTS "${CMAKE_SOURCE_DIR}/${DLL}")
        install(FILES ${DLL}
            DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
endforeach()

# Install Dawn CMake files
install(DIRECTORY deps/dawn/lib/cmake/Dawn/
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Dawn
)

# Note: Not using install(EXPORT) to avoid issues with PRIVATE dependencies
# Instead, ClipEngineConfig.cmake manually creates the IMPORTED target

# Configure package config files
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
set(SHARE_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/clipengine)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ClipEngineConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ClipEngineConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ClipEngine
    PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR SHARE_INSTALL_DIR
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ClipEngineConfigVersion.cmake"
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)

# Install CMake config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ClipEngineConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ClipEngineConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ClipEngine
)

# Backward compatibility alias
add_library(clipengine_lib ALIAS clipengine)

# ============================================================================
# Post-build SDK generation
# ============================================================================

# Add custom target to automatically install SDK after building
# Each configuration will install to its own directory
add_custom_target(install_sdk ALL
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config $<CONFIG> --prefix ${CMAKE_SOURCE_DIR}/output/ClipEngineSDK-$<CONFIG>
    DEPENDS clipengine clipengine_debug
    COMMENT "Installing ClipEngine SDK to output/ClipEngineSDK-$<CONFIG>"
)

# Make sure libraries are built before installing
add_dependencies(install_sdk clipengine clipengine_debug)

# ============================================================================
# Examples
# ============================================================================

# Add example subdirectory (optional, can be disabled with -DBUILD_EXAMPLES=OFF)
option(BUILD_EXAMPLES "Build example projects" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(example)
endif()