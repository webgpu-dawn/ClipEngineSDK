#include "CeHelper.h"

NativeWindow CeHelper::getSurfaceFromWndName(WGPUInstance instance, const char* name)
{
    NativeWindow native {};
#if _WIN32
    // 获取指定名字的窗口句柄
    HWND hwnd = FindWindow(NULL, name);
    if(!hwnd) {
        LOG_ERROR("Failed to find window : {}", name);
        return native;
    }

    // 获取客户区大小
    RECT rect;
    if(!GetClientRect(hwnd, &rect)) {
        LOG_ERROR("Failed to get client rect for {}", name);
        return native;
    }

    native.handle = hwnd;
    native.width  = rect.right - rect.left;
    native.height = rect.bottom - rect.top;

    WGPUSurfaceDescriptorFromWindowsHWND  source = {
        .chain = {
            .next = NULL,
            .sType = WGPUSType_SurfaceSourceWindowsHWND
        },
        .hinstance = GetModuleHandle(NULL),
        .hwnd = hwnd
    };

    std::string label = "ClipEngine Surface";
    WGPUSurfaceDescriptor desc = {
        .nextInChain = &source.chain,
        .label = {
            .data = label.c_str(),
            .length = label.length()
        }
    };
    native.surface = wgpu::Surface(wgpuInstanceCreateSurface(instance, &desc));
    if(!native.surface) {
        LOG_ERROR("Failed to create surface");
    }

    return native;
#elif __APPLE__
#endif
}