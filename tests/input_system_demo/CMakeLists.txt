cmake_minimum_required(VERSION 3.15)
project(InputSystemDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use same vcpkg toolchain as parent project (Windows)
if(WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")
endif()

# Find ClipEngine source directory
set(CLIPENGINE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src/clipengine")

# Check if ClipEngine source exists
if(NOT EXISTS "${CLIPENGINE_SRC_DIR}")
    message(FATAL_ERROR "ClipEngine source directory not found at: ${CLIPENGINE_SRC_DIR}")
endif()

# Find GLFW
find_package(glfw3 CONFIG)
if(NOT glfw3_FOUND)
    # Try to find GLFW in deps directory
    set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../deps/glfw")
    if(EXISTS "${GLFW_DIR}")
        message(STATUS "Using GLFW from: ${GLFW_DIR}")
        add_subdirectory(${GLFW_DIR} ${CMAKE_BINARY_DIR}/glfw EXCLUDE_FROM_ALL)
    else()
        message(FATAL_ERROR "GLFW3 not found. Please install GLFW3 via vcpkg or system package manager.")
    endif()
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)

# Create InputSystem test executable
# Use Test_ prefix when built as subdirectory for better organization in Visual Studio
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # Standalone build
    set(DEMO_TARGET_NAME input_system_demo)
else()
    # Built as subdirectory - add test prefix
    set(DEMO_TARGET_NAME Test_input_system_demo)
endif()

add_executable(${DEMO_TARGET_NAME}
    main.cpp
    ${CLIPENGINE_SRC_DIR}/input/InputSystem.cpp
)

# Include directories
target_include_directories(${DEMO_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${OPENGL_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(${DEMO_TARGET_NAME} PRIVATE
    glfw
    ${OPENGL_LIBRARIES}
)

# Platform-specific settings
if(WIN32)
    # Windows specific
    target_compile_definitions(${DEMO_TARGET_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
    )
elseif(APPLE)
    # macOS specific
    target_link_libraries(${DEMO_TARGET_NAME} PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif(UNIX)
    # Linux specific
    target_link_libraries(${DEMO_TARGET_NAME} PRIVATE
        ${CMAKE_DL_LIBS}
        pthread
    )
endif()

# Set output directory and Visual Studio working directory
# When built as subdirectory, output to parent's tests folder
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set_target_properties(${DEMO_TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
else()
    set_target_properties(${DEMO_TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/tests"
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/tests"
    )
endif()

message(STATUS "InputSystemDemo configured successfully")
message(STATUS "  ClipEngine source: ${CLIPENGINE_SRC_DIR}")
message(STATUS "  GLFW found: ${glfw3_FOUND}")
message(STATUS "  OpenGL found: ${OPENGL_FOUND}")
