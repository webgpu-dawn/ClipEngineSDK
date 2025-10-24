#pragma once

#include "../utils/Common.h"
#include "../utils/RenderUtils.h"

class RenderDevice
{
public:
    RenderDevice() = default;
    ~RenderDevice();

    bool initialize(const DeviceConfig& config);
    void shutdown();

    // Getters
    wgpu::Instance getInstance() const { return instance_; }
    wgpu::Adapter  getAdapter()  const { return adapter_; }
    wgpu::Device   getDevice()   const { return device_; }
    wgpu::Queue    getQueue()    const { return queue_; }
    wgpu::Surface  getSurface()  const { return native_.surface; }
    wgpu::TextureFormat getSurfaceFormat() const { return surface_format_; }
    const NativeWindow& getNativeWindow() const { return native_; }

    // 重新配置 surface 尺寸
    void reconfigureSurface(uint32_t width, uint32_t height);

private:
    bool initializeWebGPU(const DeviceConfig& config);

private:
    wgpu::Instance instance_;
    wgpu::Adapter  adapter_;
    wgpu::Device   device_;
    wgpu::Queue    queue_;
    wgpu::TextureFormat surface_format_ = wgpu::TextureFormat::BGRA8Unorm;

    NativeWindow native_;
};
