#pragma once
#include "Common.h"

class CeHelper
{
public:
    CeHelper() = default;
    ~CeHelper() = default;

    static NativeWindow getSurface(WGPUInstance instance, const char* name, HWND hwnd = NULL);

private:
};
