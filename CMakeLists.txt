cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0111 NEW)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_TOOLCHAIN_FILE "D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")
project(clipengine)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/deps/dawn/lib/cmake)
find_package(Dawn REQUIRED)
find_package(websocketpp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

find_package(nlohmann_json CONFIG REQUIRED)
# 禁用 nlohmann-json 的隐式转换
set(nlohmann-json_IMPLICIT_CONVERSIONS OFF)

add_subdirectory(deps/glfw)

include_directories(
    deps/dawn/include
    ${PROJECT_SOURCE_DIR}/src
)
add_subdirectory(deps/glfw3webgpu)

add_executable(
    clipengine 
    main.cpp

    src/core/util/WGPUHelpers.cpp
    src/core/util/ComboRenderPipelineDescriptor.cpp
    src/core/util/ClipContext.cpp
    src/core/renderer/TriangleRenderer.cpp
    src/core/renderer/TextureRenderer.cpp
    src/core/renderer/EquirectangularRenderer.cpp
    src/core/ClipEngine.cpp

    src/Application.cpp
    src/EventChannel.cpp
)

target_link_libraries(
    clipengine 
    dawn::webgpu_dawn
    glfw
    glfw3webgpu
    websocketpp::websocketpp
    nlohmann_json::nlohmann_json
    glm::glm
)

set_directory_properties(PROPERTIES VS_STARTUP_PROJECT clipengine)