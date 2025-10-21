#pragma once
#include "Common.h"

class CeHelper
{
public:
    CeHelper() = default;
    ~CeHelper() = default;

    static NativeWindow getSurfaceFromWndName(WGPUInstance instance, const char* name);
    static NativeWindow getSurfaceFromHWND(wgpu::Instance instance, HWND hwnd);

private:
};
