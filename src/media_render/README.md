# MediaRender SDK

一个基于 WebGPU/Dawn 的专业级高性能音视频渲染引擎 SDK。

## ✨ 核心特性

- 🚀 **完全独立** - 内置完整的 WebGPU 上下文管理，无需外部依赖
- ⚡ **GPU 零拷贝** - 直接从 D3D11 硬解码纹理渲染，无 CPU 内存拷贝
- 🎬 **NV12 原生支持** - 硬件加速的双平面 YUV 4:2:0 格式处理
- 🎨 **多层渲染系统** - 支持多个渲染器分层叠加，实现画中画等效果
- 🔧 **模块化设计** - 清晰的渲染器接口，易于扩展自定义渲染器
- 📱 **Viewport 系统** - 灵活的视口控制，支持任意区域渲染

## 架构

```
media_render/
├── core/               # 核心接口和引擎
│   ├── MediaRenderer.h     # 渲染器基类和工厂
│   ├── MediaRenderer.cpp   # 渲染器工厂实现
│   ├── RenderEngine.h      # 渲染引擎
│   └── RenderEngine.cpp    # 渲染引擎实现
└── renderers/          # 具体渲染器实现
    ├── VideoRenderer.h     # 视频渲染器
    └── VideoRenderer.cpp   # 视频渲染器实现
```

## 🚀 快速开始

### 完整示例：视频播放器

```cpp
#include "media_render/core/RenderEngine.h"
#include "media_render/renderers/VideoRenderer.h"
#include <iostream>

int main() {
    // 1. 配置并初始化渲染引擎
    MediaRender::RenderEngineConfig config;
    config.width = 800;
    config.height = 600;
    config.title = "My Video Player";

    MediaRender::RenderEngine engine;
    if (!engine.initialize(config)) {
        std::cerr << "Failed to initialize engine" << std::endl;
        return -1;
    }

    // 2. 创建并配置视频渲染器
    auto videoRenderer = std::make_unique<MediaRender::VideoRenderer>();
    videoRenderer->setViewport(0.0f, 0.0f, 1.0f, 1.0f);  // 全屏

    // 保存指针以便后续更新帧
    MediaRender::VideoRenderer* videoRendererPtr = videoRenderer.get();

    // 3. 添加到引擎
    engine.addRenderer(std::move(videoRenderer));

    // 4. 初始化视频解码器（使用 FFmpeg + D3D11 硬解码）
    Decoder decoder;
    decoder.open_video("video.mp4", [&](AVFrame* frame) {
        if (frame->format == AV_PIX_FMT_D3D11) {
            ID3D11Texture2D* texture = (ID3D11Texture2D*)frame->data[0];
            int arrayIndex = (int)(intptr_t)frame->data[1];

            // 更新视频帧
            videoRendererPtr->updateFrame(texture, arrayIndex);

            // 渲染当前帧
            engine.renderFrame();
        }
    });

    // 5. 主循环
    while (!engine.shouldClose()) {
        engine.pollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // 6. 清理
    engine.shutdown();
    return 0;
}
```

## 📦 集成到项目

### CMakeLists.txt

```cmake
# 添加 MediaRender SDK 源文件
set(MEDIARENDER_DIR ${CMAKE_SOURCE_DIR}/src/media_render)

add_executable(your_app
    main.cpp

    # MediaRender SDK
    ${MEDIARENDER_DIR}/core/MediaRenderer.cpp
    ${MEDIARENDER_DIR}/core/RenderEngine.cpp
    ${MEDIARENDER_DIR}/renderers/VideoRenderer.cpp
)

# 包含目录
target_include_directories(your_app PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${DAWN_INCLUDE_DIR}
)

# 链接库
target_link_libraries(your_app PRIVATE
    dawn::webgpu_dawn
    glfw
    glfw3webgpu
    d3d11
    dxgi
)
```

## 🎨 高级用法

### 画中画 (Picture-in-Picture)

```cpp
// 主视频（全屏）
auto mainVideo = std::make_unique<MediaRender::VideoRenderer>();
mainVideo->setViewport(0.0f, 0.0f, 1.0f, 1.0f);
mainVideo->setLayer(0);  // 底层
MediaRender::VideoRenderer* mainPtr = mainVideo.get();
engine.addRenderer(std::move(mainVideo));

// 画中画视频（右下角 25% 大小）
auto pipVideo = std::make_unique<MediaRender::VideoRenderer>();
pipVideo->setViewport(0.7f, 0.7f, 0.25f, 0.25f);
pipVideo->setLayer(1);  // 上层
MediaRender::VideoRenderer* pipPtr = pipVideo.get();
engine.addRenderer(std::move(pipVideo));

// 分别更新两个视频
mainPtr->updateFrame(mainTexture, mainIndex);
pipPtr->updateFrame(pipTexture, pipIndex);
```

### 控制渲染器状态

```cpp
// 获取渲染器
auto* videoRenderer = engine.getRenderer<MediaRender::VideoRenderer>();

if (videoRenderer) {
    // 暂停渲染
    videoRenderer->setEnabled(false);

    // 恢复渲染
    videoRenderer->setEnabled(true);

    // 改变渲染区域
    videoRenderer->setViewport(0.0f, 0.0f, 0.5f, 0.5f);

    // 改变层级
    videoRenderer->setLayer(5);
}
```

### 动态管理渲染器

```cpp
// 移除特定渲染器
auto* oldRenderer = engine.getRenderer<MediaRender::VideoRenderer>();
if (oldRenderer) {
    engine.removeRenderer(oldRenderer);
}

// 清空所有渲染器
engine.clear();

// 重新添加渲染器
auto newRenderer = std::make_unique<MediaRender::VideoRenderer>();
engine.addRenderer(std::move(newRenderer));
```

## 📚 API 参考

### RenderEngine (渲染引擎)

#### 初始化和生命周期
```cpp
bool initialize(const RenderEngineConfig& config);  // 初始化引擎（创建窗口、WebGPU上下文）
void shutdown();                                     // 关闭引擎并释放资源
bool shouldClose() const;                            // 检查窗口是否应该关闭
void pollEvents();                                   // 处理窗口事件
```

#### 渲染器管理
```cpp
void addRenderer(std::unique_ptr<IMediaRenderer> renderer);  // 添加渲染器
void removeRenderer(IMediaRenderer* renderer);               // 移除渲染器
T* getRenderer<T>();                                          // 获取指定类型的渲染器
std::vector<T*> getRenderers<T>();                          // 获取所有指定类型的渲染器
IMediaRenderer* getRendererByType(RendererType type);       // 按类型获取渲染器
void clear();                                                 // 清空所有渲染器
```

#### 渲染控制
```cpp
void renderFrame();                          // 渲染一帧（自动管理RenderPass）
void update(float deltaTime);                // 更新所有渲染器
void setBackgroundColor(float r, g, b, a);   // 设置背景色
```

#### 访问器
```cpp
wgpu::Device getDevice() const;           // 获取WebGPU设备
wgpu::Queue getQueue() const;             // 获取命令队列
wgpu::TextureFormat getSurfaceFormat() const;  // 获取Surface格式
GLFWwindow* getWindow() const;            // 获取GLFW窗口
```

### RenderEngineConfig (引擎配置)
```cpp
struct RenderEngineConfig {
    uint32_t width = 800;                    // 窗口宽度
    uint32_t height = 600;                   // 窗口高度
    const char* title = "MediaRender Engine";  // 窗口标题
    bool vsync = true;                       // 垂直同步
    wgpu::PresentMode presentMode = wgpu::PresentMode::Fifo;  // 呈现模式
};
```

### IMediaRenderer (渲染器基类)

#### 核心方法
```cpp
virtual bool initialize(wgpu::Device device, wgpu::TextureFormat format) = 0;
virtual void render(wgpu::RenderPassEncoder& pass) = 0;
virtual void update(float deltaTime) = 0;
virtual RendererType getType() const = 0;
virtual void setViewport(float x, float y, float width, float height) = 0;
```

#### 状态控制
```cpp
void setEnabled(bool enabled);    // 启用/禁用渲染
bool isEnabled() const;           // 检查是否启用
void setLayer(int layer);         // 设置渲染层级（越小越先渲染）
int getLayer() const;             // 获取层级
```

### VideoRenderer (视频渲染器)

#### 核心功能
```cpp
bool updateFrame(ID3D11Texture2D* texture, int arrayIndex = 0);  // 更新视频帧（GPU零拷贝）
void setVideoFormat(VideoFormat format);                         // 设置视频格式
void setViewport(float x, float y, float width, float height);  // 设置渲染区域
```

#### Viewport 坐标系统

`setViewport(x, y, width, height)` 参数说明：
- **`x, y`**：显示区域的**左下角**位置（归一化坐标 0.0-1.0）
- **`width, height`**：显示区域的宽度和高度（归一化坐标 0.0-1.0）
- **窗口原点**：位于左下角 `(0, 0)`

```
窗口坐标系统（左下角为原点）：

(0, 1) +-----------------+ (1, 1)  ← 左上角 | 右上角
       |                 |
       |                 |
       |                 |
(0, 0) +-----------------+ (1, 0)  ← 左下角 | 右下角
       ↑                 ↑
     原点              (1, 0)
```

**示例**：
```cpp
// 全屏
videoRenderer->setViewport(0.0f, 0.0f, 1.0f, 1.0f);
// 左下角在(0.0, 0.0)，右上角在(1.0, 1.0)

// 左下角 1/4 屏幕
videoRenderer->setViewport(0.0f, 0.0f, 0.5f, 0.5f);
// 左下角在(0.0, 0.0)，右上角在(0.5, 0.5)

// 左上角 1/4 屏幕
videoRenderer->setViewport(0.0f, 0.5f, 0.5f, 0.5f);
// 左下角在(0.0, 0.5)，右上角在(0.5, 1.0)

// 右下角 1/4 屏幕
videoRenderer->setViewport(0.5f, 0.0f, 0.5f, 0.5f);
// 左下角在(0.5, 0.0)，右上角在(1.0, 0.5)

// 右上角 1/4 屏幕
videoRenderer->setViewport(0.5f, 0.5f, 0.5f, 0.5f);
// 左下角在(0.5, 0.5)，右上角在(1.0, 1.0)

// 右下角画中画 (25%大小，距离边缘5%)
videoRenderer->setViewport(0.7f, 0.05f, 0.25f, 0.25f);
// 左下角在(0.7, 0.05)，右上角在(0.95, 0.3)

// 中心 1/4 屏幕
videoRenderer->setViewport(0.25f, 0.25f, 0.5f, 0.5f);
// 左下角在(0.25, 0.25)，右上角在(0.75, 0.75)
```

#### 填充模式
```cpp
enum class FillMode {
    Fit,      // 适应（保持宽高比，可能有黑边）
    Fill,     // 填充（保持宽高比，可能裁剪）
    Stretch   // 拉伸（填满视口，可能变形）
};
void setFillMode(FillMode mode);
```

#### 支持的格式
```cpp
enum class VideoFormat {
    NV12,   // 双平面 YUV 4:2:0（主要支持）
    I420,   // 三平面 YUV 4:2:0
    RGBA    // RGBA 8位
};
```

## ⚡ 性能优化技术

### GPU 零拷贝架构
- 使用 `SharedTextureMemory` + DXGI 共享句柄
- D3D11 硬解码纹理 → Dawn WebGPU 直接访问
- 完全避免 CPU 内存拷贝和 GPU→CPU→GPU 往返

### 纹理管理
- 使用静态变量管理共享纹理生命周期
- 相同尺寸视频帧自动复用共享纹理
- 仅在分辨率变化时重新创建纹理

### 渲染优化
- 渲染器按 Layer 自动排序，优化渲染顺序
- 支持禁用不需要的渲染器，节省 GPU 资源
- 单一 RenderPass 完成所有渲染，减少状态切换

### 色彩空间转换
- 使用 GPU Shader 进行 YUV→RGB 转换
- BT.709 标准色彩空间转换矩阵
- 完全在 GPU 端完成，零 CPU 开销

## 💻 系统要求

### 操作系统
- Windows 10 1809+ (with D3D12 support)
- Windows 11

### 硬件要求
- 支持 D3D11 或 D3D12 的 GPU
- NVIDIA / AMD / Intel 集成显卡

### 软件依赖
- **Dawn** - Google 的 WebGPU 实现
- **GLFW** 3.3+ - 窗口管理
- **glfw3webgpu** - GLFW/WebGPU 集成
- **C++17** 或更高版本编译器

## 🗂️ 文件结构

```
media_render/
├── core/
│   ├── MediaRenderer.h      # 渲染器基类接口定义和工厂
│   ├── MediaRenderer.cpp    # 渲染器工厂实现
│   ├── RenderEngine.h       # 渲染引擎（独立，包含完整WebGPU上下文）
│   └── RenderEngine.cpp     # 渲染引擎实现
│
├── renderers/
│   ├── VideoRenderer.h      # 视频渲染器（支持NV12 GPU零拷贝）
│   └── VideoRenderer.cpp    # 视频渲染器实现
│
└── README.md                # 本文档
```

**示例项目**: 参考 `example/video/` 目录中的完整视频播放器实现。

## 📝 许可证

MIT License

## 🙋 技术支持

如有问题或建议，欢迎提交 Issue 或 Pull Request。

## 🔖 版本历史

### v1.0.0 (2024)
- ✅ 完全独立的渲染引擎，内置 WebGPU 上下文管理
- ✅ GPU 零拷贝视频渲染（D3D11→Dawn）
- ✅ NV12 格式原生支持
- ✅ 多层渲染系统
- ✅ Viewport 灵活控制
