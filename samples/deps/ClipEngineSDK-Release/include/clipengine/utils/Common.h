#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <algorithm>
#include <GLFW/glfw3.h>

#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>
#include <dawn/webgpu_cpp_print.h>

#if _WIN32
#include <Windows.h>
#elif __APPLE__
#endif

struct NativeWindow {
    wgpu::Surface surface = nullptr;
    void* handle = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct DeviceConfig
{
    uint32_t width;
    uint32_t height;
    std::string window_title;
#if _WIN32
    HWND hwnd;
#elif __APPLE__
#endif
};

