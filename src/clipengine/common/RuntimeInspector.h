#pragma once

#include "Common.h"

class RuntimeInspector {
public:
    static void dumpGPUInfo(const wgpu::Adapter& adapter);
    static void dumpNativeWindowInfo(const NativeWindow& native);
    // static void PrintDeviceFeatures(const wgpu::Device& device);
    // static void PrintVertexLayout(const std::string& name, int vertexCount, int stride);
    // static void PrintRenderPipeline(const wgpu::RenderPipeline& pipeline);
    // static void PrintMemoryUsage(size_t bytes);
};
