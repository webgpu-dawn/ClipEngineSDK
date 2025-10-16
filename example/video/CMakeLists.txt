cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0111 NEW)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_TOOLCHAIN_FILE "D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "Vcpkg toolchain file")

project(video)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/../../deps/dawn/lib/cmake)
find_package(Dawn REQUIRED)
find_package(FFMPEG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src)

include_directories(
    ${SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/../../deps/dawn/include
    ${FFMPEG_INCLUDE_DIRS}
)
link_directories(
    ${FFMPEG_LIBRARY_DIRS}
)

include(FetchContent)
FetchContent_Declare(
    glfw
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../deps/glfw
)
FetchContent_MakeAvailable(glfw)

include(FetchContent)
FetchContent_Declare(
    glfw3webgpu
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../deps/glfw3webgpu
)
FetchContent_MakeAvailable(glfw3webgpu)

add_executable(video
    main.cpp
    decoder.cpp

    ${SOURCE_DIR}/media_render/core/MediaRenderer.cpp
    ${SOURCE_DIR}/media_render/core/RenderEngine.cpp
    ${SOURCE_DIR}/media_render/renderers/VideoRenderer.cpp
)

target_link_libraries(video PRIVATE
    ${FFMPEG_LIBRARIES}
    d3d11
    dxgi
    d3dcompiler
    nlohmann_json::nlohmann_json
    dawn::webgpu_dawn
    glfw
    glfw3webgpu
)
