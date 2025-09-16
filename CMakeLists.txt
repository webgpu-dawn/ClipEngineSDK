cmake_minimum_required(VERSION 3.13)

project(sample)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/deps/dawn/lib/cmake)
find_package(Dawn REQUIRED)

add_subdirectory(deps/glfw)

include_directories(
    deps/dawn/include
    ${CMAKE_SOURCE_DIR}
)
add_subdirectory(deps/glfw3webgpu)

add_executable(
    sample 
    sample.cpp
    WGPUHelpers.cpp
    ComboRenderPipelineDescriptor.cpp)

target_link_libraries(
    sample 
    dawn::webgpu_dawn
    glfw
    glfw3webgpu
)

set_directory_properties(PROPERTIES VS_STARTUP_PROJECT sample)