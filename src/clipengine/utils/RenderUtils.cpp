#include "RenderUtils.h"
#include "CeLogger.h"

using namespace wgpu;

NativeWindow RenderUtils::getSurface(WGPUInstance instance, const char* name, HWND h)
{
    NativeWindow native {};
#if _WIN32
    HWND hwnd;

    hwnd = h? h : FindWindow(NULL, name);
    
    if(!hwnd) {
        LOG_ERROR("Failed to find window : {}", name);
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

    // 获取 surface
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
    return native;
#endif
}