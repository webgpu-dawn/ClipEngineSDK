# Media Render Engine

一个基于 WebGPU/Dawn 的高性能音视频渲染引擎。

## 特性

- ✅ **GPU 零拷贝视频渲染** - 直接从 D3D11 硬解码纹理渲染
- ✅ **NV12 格式支持** - 原生支持双平面 YUV 4:2:0 格式
- ✅ **音频波形可视化** - 实时音频波形显示
- ✅ **多层渲染** - 支持多个渲染器分层组合
- ✅ **灵活的渲染管线** - 模块化设计，易于扩展

## 架构

```
media_render/
├── core/               # 核心接口和引擎
│   ├── MediaRenderer.h     # 渲染器基类
│   ├── RenderEngine.h      # 渲染引擎
│   └── RendererFactory     # 渲染器工厂
├── renderers/          # 具体渲染器实现
│   ├── VideoRenderer       # 视频渲染器
│   └── AudioWaveformRenderer # 音频波形渲染器
├── pipelines/          # 渲染管线
└── shaders/            # WGSL 着色器
```

## 快速开始

### 1. 初始化渲染引擎

```cpp
#include "media_render/core/RenderEngine.h"
#include "media_render/core/MediaRenderer.h"

using namespace MediaRender;

// 创建渲染引擎
RenderEngine engine;
engine.initialize(device, surfaceFormat);

// 设置背景色
engine.setBackgroundColor(0.0f, 0.0f, 0.0f, 1.0f);
```

### 2. 添加视频渲染器

```cpp
// 创建视频渲染器
auto videoRenderer = RendererFactory::createVideoRenderer();

// 配置渲染器
videoRenderer->setViewport(0.0f, 0.0f, 1.0f, 1.0f);  // 全屏
videoRenderer->setFillMode(VideoRenderer::FillMode::Fit);
videoRenderer->setLayer(0);  // 底层

// 添加到引擎
engine.addRenderer(std::move(videoRenderer));
```

### 3. 更新视频帧

```cpp
// 从硬解码获取视频帧
AVFrame* frame = decoder.decode();
if(frame->format == AV_PIX_FMT_D3D11) {
    ID3D11Texture2D* texture = (ID3D11Texture2D*)frame->data[0];
    int arrayIndex = (int)frame->data[1];

    // 获取视频渲染器
    auto* videoRenderer = engine.getRenderer<VideoRenderer>();
    if(videoRenderer) {
        videoRenderer->updateFrame(texture, arrayIndex);
    }
}
```

### 4. 添加音频波形渲染器

```cpp
// 创建音频波形渲染器
auto audioRenderer = RendererFactory::createAudioWaveformRenderer();

// 配置渲染器
audioRenderer->setViewport(0.0f, 0.8f, 1.0f, 0.2f);  // 底部 20%
audioRenderer->setWaveformColor(0.0f, 1.0f, 0.0f, 1.0f);  // 绿色
audioRenderer->setLayer(1);  // 上层

// 添加到引擎
engine.addRenderer(std::move(audioRenderer));
```

### 5. 更新音频数据

```cpp
// 从音频解码器获取音频样本
float* samples = audioDecoder.getSamples();
size_t sampleCount = audioDecoder.getSampleCount();
int channels = 2;  // 立体声

// 更新波形数据
auto* audioRenderer = engine.getRenderer<AudioWaveformRenderer>();
if(audioRenderer) {
    audioRenderer->updateAudioData(samples, sampleCount, channels);
}
```

### 6. 渲染循环

```cpp
while(!shouldClose) {
    // 更新
    float deltaTime = getDeltaTime();
    engine.update(deltaTime);

    // 获取当前帧纹理
    wgpu::TextureView view = surface.GetCurrentTextureView();

    // 创建 RenderPass
    wgpu::RenderPassColorAttachment attachment = {};
    attachment.view = view;
    attachment.loadOp = wgpu::LoadOp::Clear;
    attachment.storeOp = wgpu::StoreOp::Store;
    attachment.clearValue = {0.0f, 0.0f, 0.0f, 1.0f};

    wgpu::RenderPassDescriptor passDesc = {};
    passDesc.colorAttachmentCount = 1;
    passDesc.colorAttachments = &attachment;

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);

    // 渲染所有启用的渲染器
    engine.render(pass);

    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);

    surface.Present();
}
```

## 高级用法

### 多视频画中画

```cpp
// 主视频
auto mainVideo = RendererFactory::createVideoRenderer();
mainVideo->setViewport(0.0f, 0.0f, 1.0f, 1.0f);
mainVideo->setLayer(0);
engine.addRenderer(std::move(mainVideo));

// 画中画视频
auto pipVideo = RendererFactory::createVideoRenderer();
pipVideo->setViewport(0.7f, 0.7f, 0.25f, 0.25f);  // 右下角
pipVideo->setLayer(1);
engine.addRenderer(std::move(pipVideo));
```

### 启用/禁用渲染器

```cpp
auto* videoRenderer = engine.getRenderer<VideoRenderer>();
if(videoRenderer) {
    videoRenderer->setEnabled(false);  // 暂停视频渲染
}
```

### 动态切换渲染器

```cpp
// 移除旧渲染器
engine.removeRenderer(oldRenderer);

// 添加新渲染器
engine.addRenderer(RendererFactory::createVideoRenderer());
```

## API 参考

### IMediaRenderer (基类)

- `bool initialize(device, format)` - 初始化渲染器
- `void render(pass)` - 渲染
- `void update(deltaTime)` - 更新状态
- `void setViewport(x, y, width, height)` - 设置渲染区域
- `void setEnabled(bool)` - 启用/禁用
- `void setLayer(int)` - 设置渲染层级

### VideoRenderer

- `bool updateFrame(texture, arrayIndex)` - 更新视频帧
- `void setVideoFormat(format)` - 设置视频格式
- `void setFillMode(mode)` - 设置填充模式
  - `Fit` - 适应（保持比例）
  - `Fill` - 填充（可能裁剪）
  - `Stretch` - 拉伸（可能变形）

### AudioWaveformRenderer

- `void updateAudioData(samples, count, channels)` - 更新音频数据
- `void setWaveformColor(r, g, b, a)` - 设置波形颜色
- `void setDisplayMode(mode)` - 设置显示模式
  - `Waveform` - 波形
  - `Spectrum` - 频谱
  - `Both` - 两者

### RenderEngine

- `bool initialize(device, format)` - 初始化引擎
- `void addRenderer(renderer)` - 添加渲染器
- `void removeRenderer(renderer)` - 移除渲染器
- `void render(pass)` - 渲染所有渲染器
- `void update(deltaTime)` - 更新所有渲染器
- `T* getRenderer<T>()` - 获取指定类型的渲染器
- `void clear()` - 清空所有渲染器

## 性能优化

1. **GPU 零拷贝** - 视频纹理直接从 D3D11 共享到 Dawn，无 CPU 拷贝
2. **纹理复用** - 相同尺寸的视频帧复用共享纹理
3. **层级排序** - 渲染器按层级排序，减少状态切换
4. **延迟初始化** - 渲染资源按需创建

## 系统要求

- Windows 10/11
- D3D11/D3D12 支持
- Dawn (WebGPU 实现)
- C++17 或更高

## 许可证

MIT License
