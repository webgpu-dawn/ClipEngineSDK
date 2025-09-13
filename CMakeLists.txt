cmake_minimum_required(VERSION 3.13)

project(
    cfengine 
    VERSION 0.1.0 
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(deps/glfw)
add_subdirectory(deps/webgpu)
add_subdirectory(deps/glfw3webgpu)

include_directories({PROJECT_SOURCE_DIR})

add_executable(
    app 
    main.cpp 
    application.cpp
    webgpu-utils.cpp
    implementations.cpp
)

target_link_libraries(
    app PRIVATE
    glfw
    glfw3webgpu
    webgpu
)
target_copy_webgpu_binaries(app)

set_directory_properties(PROPERTIES VS_STARTUP_PROJECT app)
