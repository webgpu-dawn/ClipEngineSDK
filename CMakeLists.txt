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

# Force static linking for fmt
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
find_package(fmt CONFIG REQUIRED)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

find_package(spdlog CONFIG REQUIRED)

# Set FFmpeg paths from deps directory
set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/samples/deps/ffmpeg_x64-windows")

# FFmpeg include directories
set(FFMPEG_INCLUDE_DIRS "${FFMPEG_ROOT}/include")

# FFmpeg libraries based on build configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(FFMPEG_LIB_DIR "${FFMPEG_ROOT}/debug/lib")
else()
    set(FFMPEG_LIB_DIR "${FFMPEG_ROOT}/lib")
endif()

set(FFMPEG_LIBRARIES
    "${FFMPEG_LIB_DIR}/avcodec.lib"
    "${FFMPEG_LIB_DIR}/avformat.lib"
    "${FFMPEG_LIB_DIR}/avutil.lib"
    "${FFMPEG_LIB_DIR}/swscale.lib"
)

message(STATUS "FFmpeg root: ${FFMPEG_ROOT}")
message(STATUS "FFmpeg include: ${FFMPEG_INCLUDE_DIRS}")
message(STATUS "FFmpeg libraries: ${FFMPEG_LIBRARIES}")

# ImGui for debug module (using local deps)
set(imgui_SOURCE_DIR "${CMAKE_SOURCE_DIR}/deps/imgui")
if(NOT EXISTS "${imgui_SOURCE_DIR}/imgui.h")
    message(FATAL_ERROR "ImGui not found in deps/imgui. Please copy imgui source to deps/imgui/")
endif()
message(STATUS "Using local ImGui from: ${imgui_SOURCE_DIR}")

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

    # Transform (New Architecture)
    src/clipengine/transform/Transform2D.cpp

    # Layers (New Architecture)
    src/clipengine/layers/Layer.cpp
    src/clipengine/layers/VideoLayer.cpp
    src/clipengine/layers/ImageLayer.cpp

    # Layers (Old - kept for backward compatibility)
    src/clipengine/layers/TextureRenderer.cpp
    src/clipengine/layers/VideoRenderer.cpp

    # Effects (New Architecture)
    src/clipengine/effects/ColorAdjustEffect.cpp

    # Effects (Old - kept for backward compatibility)
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

    # Input
    src/clipengine/input/InputSystem.cpp

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

    # Transform (New Architecture)
    src/clipengine/transform/Transform2D.h

    # Layers (New Architecture)
    src/clipengine/layers/Layer.h
    src/clipengine/layers/VideoLayer.h
    src/clipengine/layers/ImageLayer.h
    src/clipengine/layers/SolidLayer.h

    # Layers (Old - backward compatibility)
    src/clipengine/layers/TextureRenderer.h
    src/clipengine/layers/VideoRenderer.h

    # Effects (New Architecture)
    src/clipengine/effects/Effect.h
    src/clipengine/effects/ColorAdjustEffect.h

    # Effects (Old - backward compatibility)
    src/clipengine/effects/Filter.h
    src/clipengine/effects/FilterChain.h
    src/clipengine/effects/ShaderEffect.h
    src/clipengine/effects/EffectLoader.h

    # Compositor (New Architecture)
    src/clipengine/compositor/Compositor.h

    # Simple API (New Architecture)
    src/clipengine/SimpleAPI.h

    # Shaders
    src/clipengine/shaders/ShaderLibrary.h
    src/clipengine/shaders/ShaderConfig.h

    # Export
    src/clipengine/export/VideoExporter.h

    # Input
    src/clipengine/input/InputEvent.h
    src/clipengine/input/InputSystem.h
    src/clipengine/input/InputSystemGLFWAdapter.h
    src/clipengine/input/InputSystemWin32Adapter.h
    src/clipengine/input/KeyCode.h

    # Utilities
    src/clipengine/utils/Common.h
    src/clipengine/utils/RenderUtils.h

    # Note: Debug headers (DebugWindow) are NOT exported
    # They are optional and available as source code only
)

# Configure library properties
target_include_directories(clipengine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/deps/dawn/include>
        $<BUILD_INTERFACE:${FFMPEG_INCLUDE_DIRS}>
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
        glm::glm
    PRIVATE
        spdlog::spdlog_header_only
        fmt::fmt-header-only
        ${FFMPEG_LIBRARIES}
)

# Alias for consistent naming
add_library(ClipEngine::clipengine ALIAS clipengine)

# ============================================================================
# ClipEngine Debug Library (Optional)
# ============================================================================
# Separate library for debug features to avoid forcing ImGui dependency on all users

# Find GLFW from samples deps (or system)
set(GLFW_SOURCE_DIR "${CMAKE_SOURCE_DIR}/samples/deps/glfw")
if(EXISTS "${GLFW_SOURCE_DIR}/CMakeLists.txt")
    # Force GLFW to build as static library
    set(BUILD_SHARED_LIBS_SAVE ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)

    add_subdirectory(${GLFW_SOURCE_DIR} ${CMAKE_BINARY_DIR}/glfw EXCLUDE_FROM_ALL)

    # Restore BUILD_SHARED_LIBS
    set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_SAVE} CACHE BOOL "" FORCE)
    message(STATUS "Using local GLFW (static) from: ${GLFW_SOURCE_DIR}")
else()
    find_package(glfw3 REQUIRED)
    message(STATUS "Using system GLFW")
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
install(TARGETS clipengine clipengine_debug imgui_core imgui_backends glfw
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

# Install GLFW headers
if(EXISTS "${GLFW_SOURCE_DIR}/include")
    install(DIRECTORY ${GLFW_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
    )
endif()

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
# Examples/Samples
# ============================================================================

# Option to build samples (default: ON)
option(BUILD_SAMPLES "Build sample projects" ON)
option(BUILD_TESTS "Build test projects" ON)

if(BUILD_SAMPLES)
    message(STATUS "Building samples as subdirectories")

    # Add samples as subdirectory - sdk_demo is now directly in samples/
    add_subdirectory(samples)

    message(STATUS "Samples added as subdirectories and will appear as launchable targets in Visual Studio")
    message(STATUS "  - Sample_sdk_demo: Complete SDK demonstration with video rendering")
endif()

if(BUILD_TESTS)
    message(STATUS "Building tests as subdirectories")

    # Add tests as subdirectories
    add_subdirectory(tests/input_system_demo)

    message(STATUS "Tests added as subdirectories and will appear as launchable targets in Visual Studio")
    message(STATUS "  - Test_input_system_demo: Input system test demonstration")
endif()

# OLD CODE COMMENTED OUT - keeping for reference
if(FALSE)
    # Custom target to build input_system_demo (NO LONGER USED)
    add_custom_target(Sample_input_system_demo_OLD
        COMMAND ${CMAKE_COMMAND} -E echo "Building input_system_demo..."
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/samples/input_system_demo/build
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR}/samples/input_system_demo
                                 -B ${CMAKE_SOURCE_DIR}/samples/input_system_demo/build
                                 -G "${CMAKE_GENERATOR}"
                                 ${PLATFORM_ARG}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_SOURCE_DIR}/samples/input_system_demo/build --config $<CONFIG>
        COMMAND ${CMAKE_COMMAND} -E echo "Copying input_system_demo.exe to build directory..."
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/$<CONFIG>/samples
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/samples/input_system_demo/build/bin/$<CONFIG>/input_system_demo.exe
                ${CMAKE_BINARY_DIR}/$<CONFIG>/samples/Sample_input_system_demo.exe
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Building input_system_demo sample"
    )

    # Custom target to build sdk_demo (depends on SDK being installed)
    add_custom_target(Sample_sdk_demo
        COMMAND ${CMAKE_COMMAND} -E echo "Building sdk_demo..."
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/samples/sdk_demo/build
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR}/samples/sdk_demo
                                 -B ${CMAKE_SOURCE_DIR}/samples/sdk_demo/build
                                 -G "${CMAKE_GENERATOR}"
                                 ${PLATFORM_ARG}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_SOURCE_DIR}/samples/sdk_demo/build --config $<CONFIG>
        COMMAND ${CMAKE_COMMAND} -E echo "Copying sdk_demo.exe to build directory..."
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/$<CONFIG>/samples
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/samples/sdk_demo/build/$<CONFIG>/sdk_demo.exe
                ${CMAKE_BINARY_DIR}/$<CONFIG>/samples/Sample_sdk_demo.exe
        COMMAND ${CMAKE_COMMAND} -E echo "Copying required DLLs for sdk_demo..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/samples/sdk_demo/build/$<CONFIG>/webgpu_dawn.dll
                ${CMAKE_BINARY_DIR}/$<CONFIG>/samples/webgpu_dawn.dll
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/samples/sdk_demo/build/$<CONFIG>/d3dcompiler_47.dll
                ${CMAKE_BINARY_DIR}/$<CONFIG>/samples/d3dcompiler_47.dll
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/samples/sdk_demo/build/$<CONFIG>/vulkan-1.dll
                ${CMAKE_BINARY_DIR}/$<CONFIG>/samples/vulkan-1.dll
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/samples/sdk_demo/build/$<CONFIG>/shaders
                ${CMAKE_BINARY_DIR}/$<CONFIG>/samples/shaders
        COMMAND ${CMAKE_COMMAND} -E echo "Copying FFmpeg DLLs for sdk_demo..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/samples/sdk_demo/deps/ffmpeg_x64-windows/bin/avcodec-61.dll
                ${CMAKE_BINARY_DIR}/$<CONFIG>/samples/avcodec-61.dll
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/samples/sdk_demo/deps/ffmpeg_x64-windows/bin/avformat-61.dll
                ${CMAKE_BINARY_DIR}/$<CONFIG>/samples/avformat-61.dll
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/samples/sdk_demo/deps/ffmpeg_x64-windows/bin/avutil-59.dll
                ${CMAKE_BINARY_DIR}/$<CONFIG>/samples/avutil-59.dll
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/samples/sdk_demo/deps/ffmpeg_x64-windows/bin/swscale-8.dll
                ${CMAKE_BINARY_DIR}/$<CONFIG>/samples/swscale-8.dll
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/samples/sdk_demo/deps/ffmpeg_x64-windows/bin/swresample-5.dll
                ${CMAKE_BINARY_DIR}/$<CONFIG>/samples/swresample-5.dll
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Building sdk_demo sample"
    )

    # Make sure sdk_demo is built after SDK is installed
    add_dependencies(Sample_sdk_demo install_sdk)

    # Aggregate target to build all samples
    add_custom_target(Sample_build_all_samples
        DEPENDS Sample_input_system_demo Sample_sdk_demo
    )

    message(STATUS "Sample build targets created:")
    message(STATUS "  - Sample_input_system_demo: Build input system demo")
    message(STATUS "  - Sample_sdk_demo: Build SDK demo (requires install_sdk)")
    message(STATUS "  - Sample_build_all_samples: Build all samples")
endif() # if(FALSE) - commented out old custom targets