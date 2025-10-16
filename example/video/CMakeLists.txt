# ============================================================================
# Video Example - Standalone test project for ClipEngine SDK
# ============================================================================

cmake_minimum_required(VERSION 3.13)
project(ClipEngineVideoExample)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_TOOLCHAIN_FILE "D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")

# Find ClipEngine SDK from output directory
# The ClipEngine project should be built and SDK generated first:
# cd D:\TestDawn && cmake -B build -S . && cmake --build build --config Debug
# cd D:\TestDawn && cmake -B build -S . && cmake --build build --config Release
#
# SDK structure:
#   output/ClipEngineSDK-Debug/    - Debug build
#   output/ClipEngineSDK-Release/  - Release build

# For multi-config generators (Visual Studio), we need to handle both Debug and Release
# Add both SDK directories to the search path
set(CLIPENGINE_SDK_BASE "${CMAKE_CURRENT_SOURCE_DIR}/../../output")
list(APPEND CMAKE_PREFIX_PATH
    "${CLIPENGINE_SDK_BASE}/ClipEngineSDK-Debug"
    "${CLIPENGINE_SDK_BASE}/ClipEngineSDK-Release"
)

# The SDK will be selected at build time based on the configuration
find_package(ClipEngine REQUIRED)

message(STATUS "Building video example as standalone test project")
message(STATUS "  ClipEngine version: ${ClipEngine_VERSION}")
message(STATUS "  ClipEngine include: ${ClipEngine_INCLUDE_DIRS}")
message(STATUS "  ClipEngine libraries: ${ClipEngine_LIBRARIES}")
message(STATUS "  ClipEngine shaders: ${ClipEngine_SHADER_DIR}")

# Find FFMPEG (installed via vcpkg)
# vcpkg provides FFMPEG libraries directly
set(FFMPEG_LIBRARIES
    avcodec
    avformat
    avutil
    swscale
)

# Get include directories from vcpkg
if(DEFINED ENV{VCPKG_ROOT})
    set(VCPKG_INSTALLED_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows")
elseif(EXISTS "D:/code/vcpkg/installed/x64-windows")
    set(VCPKG_INSTALLED_DIR "D:/code/vcpkg/installed/x64-windows")
endif()

if(VCPKG_INSTALLED_DIR)
    include_directories(${VCPKG_INSTALLED_DIR}/include)
    link_directories(${VCPKG_INSTALLED_DIR}/lib)
endif()

add_executable(video
    main.cpp
    decoder.cpp
)

# Link against ClipEngine SDK and other dependencies
target_link_libraries(video PRIVATE
    ClipEngine::clipengine
    ${FFMPEG_LIBRARIES}
    d3d11
    dxgi
    d3dcompiler
)

# Automatically copy ClipEngine runtime DLLs (webgpu_dawn.dll)
# This function is provided by ClipEngineConfig.cmake
clipengine_copy_dlls(video)

# Copy extra dependencies (d3dcompiler, vulkan)
# These should be in the source tree's deps/extra directory
set(EXTRA_DLLS
    d3dcompiler_47.dll
    vulkan-1.dll
)

set(EXTRA_DLL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../deps/extra")
foreach(DLL ${EXTRA_DLLS})
    if(EXISTS "${EXTRA_DLL_DIR}/${DLL}")
        add_custom_command(TARGET video POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${EXTRA_DLL_DIR}/${DLL}"
                $<TARGET_FILE_DIR:video>
            COMMENT "Copying ${DLL}"
        )
    endif()
endforeach()

# Copy FFMPEG DLLs to output directory (only the necessary ones)
set(FFMPEG_DLLS
    avcodec-61.dll
    avdevice-61.dll
    avfilter-10.dll
    avformat-61.dll
    avutil-59.dll
    swresample-5.dll
    swscale-8.dll
)

foreach(DLL ${FFMPEG_DLLS})
    if(EXISTS "D:/code/vcpkg/installed/x64-windows/bin/${DLL}")
        add_custom_command(TARGET video POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "D:/code/vcpkg/installed/x64-windows/bin/${DLL}"
                $<TARGET_FILE_DIR:video>
            COMMENT "Copying ${DLL}"
        )
    endif()
endforeach()

set_directory_properties(PROPERTIES VS_STARTUP_PROJECT video)
