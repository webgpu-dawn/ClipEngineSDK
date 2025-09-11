cmake_minimum_required(VERSION 3.13)

project(hello_webgpu)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/deps/dawn/lib/cmake)
find_package(Dawn REQUIRED)
add_executable(hello_webgpu hello_webgpu.cpp)

# Declare dependency on the dawn::webgpu_dawn library
target_link_libraries(hello_webgpu dawn::webgpu_dawn)