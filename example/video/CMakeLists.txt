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
# ClipEngine DLLs will be automatically copied when linking ClipEngine::clipengine
target_link_libraries(video PRIVATE
    ClipEngine::clipengine
    ${FFMPEG_LIBRARIES}
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
        add_custom_command(TARGET video POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CLIPENGINE_DLL_SOURCE}/${DLL}"
                $<TARGET_FILE_DIR:video>
            COMMENT "Copying ClipEngine DLL: ${DLL}"
        )
    elseif(EXISTS "${CLIPENGINE_EXTRA_DLL_SOURCE}/${DLL}")
        add_custom_command(TARGET video POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CLIPENGINE_EXTRA_DLL_SOURCE}/${DLL}"
                $<TARGET_FILE_DIR:video>
            COMMENT "Copying ClipEngine DLL: ${DLL}"
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
