cmake_minimum_required(VERSION 3.13)
project(sdk_demo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Determine target name based on build context
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # Standalone build - use original name
    set(DEMO_TARGET_NAME sdk_demo)
    set(BUILD_AS_SUBDIRECTORY FALSE)

    set(CLIPENGINE_SDK_BASE "${CMAKE_CURRENT_SOURCE_DIR}/deps")
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/ffmpeg_x64-windows/share/ffmpeg)
    list(APPEND CMAKE_PREFIX_PATH
        "${CLIPENGINE_SDK_BASE}/ClipEngineSDK-Debug"
        "${CLIPENGINE_SDK_BASE}/ClipEngineSDK-Release"
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/ffmpeg_x64-windows"
    )

    # The SDK will be selected at build time based on the configuration
    find_package(ClipEngine REQUIRED)
else()
    # Built as subdirectory - add prefix and link directly
    set(DEMO_TARGET_NAME Sample_sdk_demo)
    set(BUILD_AS_SUBDIRECTORY TRUE)

    # When built as subdirectory, ClipEngine is already available
    # No need to find_package
endif()

# Set FFmpeg paths directly from deps directory
set(FFMPEG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/ffmpeg_x64-windows")

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

message(STATUS "FFmpeg include: ${FFMPEG_INCLUDE_DIRS}")
message(STATUS "FFmpeg libraries: ${FFMPEG_LIBRARIES}")

# Note: GLFW and ImGui are now provided by ClipEngine::clipengine_debug
# No need to build them again in example

message(STATUS "Building SDK demo as comprehensive example project")
message(STATUS "  ClipEngine version: ${ClipEngine_VERSION}")
message(STATUS "  ClipEngine include: ${ClipEngine_INCLUDE_DIRS}")
message(STATUS "  ClipEngine libraries: ${ClipEngine_LIBRARIES}")
message(STATUS "  ClipEngine shaders: ${ClipEngine_SHADER_DIR}")

add_executable(
    ${DEMO_TARGET_NAME}
    main.cpp
    Decoder.cpp
    VideoSource.cpp
    PanoramaController.cpp
    Application.cpp
)

# Link against ClipEngine SDK and other dependencies
# ClipEngine DLLs will be automatically copied when linking ClipEngine::clipengine
if(BUILD_AS_SUBDIRECTORY)
    # Building as subdirectory - link directly to targets
    target_link_libraries(
        ${DEMO_TARGET_NAME} PRIVATE
        clipengine
        ${FFMPEG_LIBRARIES}
        d3d11
        dxgi
        d3dcompiler
    )
    # Link debug window support
    if(TARGET clipengine_debug)
        target_link_libraries(${DEMO_TARGET_NAME} PRIVATE clipengine_debug)
        target_compile_definitions(${DEMO_TARGET_NAME} PRIVATE CLIPENGINE_DEBUG_WINDOW_ENABLED)
        message(STATUS "Debug window support: enabled")
    endif()
else()
    # Standalone build - use SDK targets
    target_link_libraries(
        ${DEMO_TARGET_NAME} PRIVATE
        ClipEngine::clipengine
        ${FFMPEG_LIBRARIES}
        d3d11
        dxgi
        d3dcompiler
    )
    # Link debug window support if available
    if(TARGET ClipEngine::clipengine_debug)
        target_link_libraries(${DEMO_TARGET_NAME} PRIVATE ClipEngine::clipengine_debug)
        target_compile_definitions(${DEMO_TARGET_NAME} PRIVATE CLIPENGINE_DEBUG_WINDOW_ENABLED)
        message(STATUS "Debug window support: enabled")
    else()
        message(STATUS "Debug window support: disabled (not available in SDK)")
    endif()
endif()

target_include_directories(
    ${DEMO_TARGET_NAME} PRIVATE
    ${FFMPEG_INCLUDE_DIRS}
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
        add_custom_command(TARGET ${DEMO_TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CLIPENGINE_DLL_SOURCE}/${DLL}"
                $<TARGET_FILE_DIR:${DEMO_TARGET_NAME}>
            COMMENT "Copying ClipEngine DLL: ${DLL}"
        )
    elseif(EXISTS "${CLIPENGINE_EXTRA_DLL_SOURCE}/${DLL}")
        add_custom_command(TARGET ${DEMO_TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CLIPENGINE_EXTRA_DLL_SOURCE}/${DLL}"
                $<TARGET_FILE_DIR:${DEMO_TARGET_NAME}>
            COMMENT "Copying ClipEngine DLL: ${DLL}"
        )
    endif()
endforeach()

# Copy shader files to output directory
if(TARGET clipengine)
    # Building from source - copy shaders directly from source directory
    add_custom_command(TARGET ${DEMO_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/src/clipengine/shaders"
            "$<TARGET_FILE_DIR:${DEMO_TARGET_NAME}>/shaders"
        COMMENT "Copying ClipEngine shader files from source"
    )
elseif(ClipEngine_SHADER_DIR AND EXISTS "${ClipEngine_SHADER_DIR}")
    # Using SDK - copy from SDK shader directory
    add_custom_command(TARGET ${DEMO_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ClipEngine_SHADER_DIR}"
            "$<TARGET_FILE_DIR:${DEMO_TARGET_NAME}>/shaders"
        COMMENT "Copying ClipEngine shader files from SDK"
    )
endif()

# Set output directory and Visual Studio working directory
if(BUILD_AS_SUBDIRECTORY)
    set_target_properties(${DEMO_TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/samples"
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/samples"
    )
else()
    set_directory_properties(PROPERTIES VS_STARTUP_PROJECT ${DEMO_TARGET_NAME})
endif()
