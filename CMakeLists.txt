cmake_minimum_required(VERSION 3.13)

project(hello_webgpu)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(deps/glfw)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/deps/dawn/lib/cmake)
find_package(Dawn REQUIRED)

include_directories(deps/dawn/include)
add_subdirectory(deps/glfw3webgpu)

add_executable(sample sample.cpp application.cpp)

target_link_libraries(
    sample 
    dawn::webgpu_dawn
    glfw
    glfw3webgpu
)