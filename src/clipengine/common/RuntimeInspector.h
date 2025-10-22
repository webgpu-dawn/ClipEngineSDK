#pragma once

#include "Common.h"

class RuntimeInspector {
public:
    static void dumpGPUInfo(const wgpu::Adapter& adapter);
    static void dumpNativeWindowInfo(const NativeWindow& native);
};
