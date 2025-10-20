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

add_subdirectory(deps/glfw)
add_subdirectory(deps/glfw3webgpu)

include_directories(
    deps/dawn/include
    ${PROJECT_SOURCE_DIR}/src
)

# ============================================================================
# ClipEngine Library - Professional Video Editing Engine Core
# ============================================================================

add_library(clipengine STATIC
    src/clipengine/ClipEngine.cpp
    src/clipengine/core/CeContext.cpp
    src/clipengine/common/RuntimeInspector.cpp
    src/clipengine/render/RenderEngine.cpp
    src/clipengine/render/VideoRenderer.cpp
    src/clipengine/render/Renderer.cpp
    # src/clipengine/core/WebGPUHelper.cpp
    src/clipengine/util/GPUTimer.cpp
    # src/clipengine/util/GPUProfiler.cpp  # Temporarily disabled
)

# Set public headers for installation
set(CLIPENGINE_PUBLIC_HEADERS
    src/clipengine/ClipEngine.h
    src/clipengine/render/Renderer.h
    src/clipengine/render/RenderEngine.h
    src/clipengine/render/VideoRenderer.h
    src/clipengine/core/WebGPUHelper.h
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
        spdlog::spdlog
    PRIVATE
        glfw
        glfw3webgpu
        glm::glm
        spdlog::spdlog_header_only
)

# Alias for consistent naming
add_library(ClipEngine::clipengine ALIAS clipengine)

# ============================================================================
# Installation Rules
# ============================================================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install library (without EXPORT to avoid exporting PRIVATE dependencies)
install(TARGETS clipengine
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Note: GLFW and glfw3webgpu are PRIVATE dependencies
# They are statically linked into clipengine.lib and not exposed to users

# Install headers with directory structure
install(DIRECTORY src/clipengine/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/clipengine
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install Dawn headers (required dependency)
install(DIRECTORY deps/dawn/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dawn
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install shaders
install(DIRECTORY src/clipengine/shader/
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

# Install GLFW and glfw3webgpu libraries (they are statically linked into clipengine)
install(TARGETS glfw glfw3webgpu
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

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
    DEPENDS clipengine
    COMMENT "Installing ClipEngine SDK to output/ClipEngineSDK-$<CONFIG>"
)

# Make sure clipengine is built before installing
add_dependencies(install_sdk clipengine)

# ============================================================================
# Examples
# ============================================================================

# Add example subdirectory (optional, can be disabled with -DBUILD_EXAMPLES=OFF)
option(BUILD_EXAMPLES "Build example projects" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(example/video)
endif()