#pragma once

#include "Common.h"

class RuntimeInspector {
public:
    static void dumpSurfaceCaps(const wgpu::Adapter& adapter, const wgpu::Surface& surface);
    static void dumpGPUInfo(const wgpu::Adapter& adapter);
    static void dumpNativeWindowInfo(const NativeWindow& native);
};
