#pragma once
#include "Common.h"

class RenderUtils
{
public:
    RenderUtils() = default;
    ~RenderUtils() = default;

    static NativeWindow getSurface(WGPUInstance instance, const char* name, HWND hwnd = NULL);

private:
};
