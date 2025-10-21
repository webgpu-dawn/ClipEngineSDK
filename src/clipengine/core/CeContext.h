#pragma once

#include "../common/Common.h"
#include "../common/CeHelper.h"

// Forward declaration
struct GLFWwindow;

enum class CeContextMode {
    CREATE_WINDOW,    // 创建新窗口
    FIND_WINDOW       // 查找已存在的窗口
};

struct CeContextConfig {
    CeContextMode mode = CeContextMode::CREATE_WINDOW;
    const char* windowTitle = "ClipEngine";
    uint32_t width = 800;
    uint32_t height = 600;
};

class CeContext
{
public:
    CeContext() = default;
    ~CeContext();

    bool initialize(const CeContextConfig& config);
    void shutdown();

    // Getters
    wgpu::Instance getInstance() const { return instance_; }
    wgpu::Adapter  getAdapter()  const { return adapter_; }
    wgpu::Device   getDevice()   const { return device_; }
    wgpu::Queue    getQueue()    const { return queue_; }
    wgpu::Surface  getSurface()  const { return native_.surface; }
    wgpu::TextureFormat getSurfaceFormat() const { return surface_format_; }
    GLFWwindow* getWindow() const { return window_; }

    // 重新配置 surface 尺寸
    void reconfigureSurface(uint32_t width, uint32_t height);

private:
    bool initializeWebGPU(const CeContextConfig& config);
    bool createWindowMode(const CeContextConfig& config);

private:
    GLFWwindow* window_ = nullptr;
    HWND hwnd_;

    wgpu::Instance instance_;
    wgpu::Adapter  adapter_;
    wgpu::Device   device_;
    wgpu::Queue    queue_;
    wgpu::TextureFormat surface_format_ = wgpu::TextureFormat::BGRA8Unorm;

    NativeWindow native_;
};
