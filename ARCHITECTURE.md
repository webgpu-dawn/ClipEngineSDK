# ClipEngine SDK - 架构设计文档

> **版本**: 1.0
> **更新日期**: 2025-10-24
> **作者**: ClipEngine Team

---

## 目录

1. [概述](#概述)
2. [设计理念](#设计理念)
3. [整体架构](#整体架构)
4. [核心组件](#核心组件)
5. [渲染流程](#渲染流程)
6. [着色器系统](#着色器系统)
7. [性能优化](#性能优化)
8. [扩展性设计](#扩展性设计)
9. [使用示例](#使用示例)
10. [未来规划](#未来规划)

---

## 概述

ClipEngine 是一个基于 **WebGPU (Dawn)** 的高性能视频渲染引擎 SDK，专为实时视频处理、后期制作和交互式媒体应用设计。采用分层架构和组件化设计，提供灵活的 API 和强大的扩展能力。

### 核心特性

- ✨ **现代图形 API**: 基于 WebGPU，跨平台支持
- 🎬 **多层合成**: 类似 After Effects 的时间线系统
- 🎨 **丰富特效**: 内置多种视觉效果，支持自定义着色器
- 🚀 **高性能**: GPU 加速，零拷贝纹理传输
- 🔧 **易扩展**: 数据驱动，插件化设计
- 📱 **跨平台**: Windows/macOS/Linux (未来支持移动端)

---

## 设计理念

### 1. 核心理念

**滤镜和特效的本质 = Shader + Uniform 参数**

这是我们架构的核心思想。所有的视觉效果都可以归结为：
1. **Shader 代码**：定义像素处理逻辑 (WGSL)
2. **Uniform 参数**：可调节的参数（亮度、对比度、模糊半径等）

基于这个理念，我们设计了一套通用、灵活、可扩展的渲染架构。

### 2. 设计原则

- **分层架构**: 清晰的职责划分，降低耦合
- **组件化**: 独立的功能模块，可组合可复用
- **数据驱动**: 配置优于代码，参数化设计
- **零拷贝**: 最小化 CPU-GPU 数据传输
- **线程安全**: 明确的线程模型，保证并发安全

---

## 整体架构

### 架构分层

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│                  (用户应用程序层)                            │
│   • 视频播放器                                               │
│   • 视频编辑器                                               │
│   • 实时直播                                                │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│              CompositionEngine (组合引擎层)                  │
│   • 多层管理 (Layer Management)                              │
│   • 全局后处理 (Global Post-Processing)                      │
│   • 输入状态管理 (Input State Management)                    │
│   • 渲染调度 (Render Scheduling)                            │
└────────────┬─────────────────┬──────────────────────────────┘
             │                 │
    ┌────────▼────────┐   ┌───▼──────────────────┐
    │  Layer System   │   │  Effect System       │
    │  (层系统)       │   │  (效果系统)          │
    └────────┬────────┘   └───┬──────────────────┘
             │                 │
    ┌────────▼─────────────────▼──────────────────┐
    │      Rendering Pipeline Layer                │
    │      (渲染管线层)                            │
    │   • ShaderLibrary (着色器库)                │
    │   • FilterChain (效果链)                    │
    │   • TextureManager (纹理管理)               │
    └────────┬────────────────────────────────────┘
             │
    ┌────────▼────────────────────────────────────┐
    │      WebGPU Abstraction Layer                │
    │      (WebGPU 抽象层)                         │
    │   • CeContext (上下文管理)                  │
    │   • Device/Queue/Surface                    │
    └────────┬────────────────────────────────────┘
             │
    ┌────────▼────────────────────────────────────┐
    │         Dawn (WebGPU 实现)                   │
    │   • Vulkan Backend                          │
    │   • D3D12 Backend                           │
    │   • Metal Backend                           │
    └─────────────────────────────────────────────┘
```

### 模块依赖关系

```
CompositionEngine
    ├─ depends on → CeContext
    ├─ depends on → FilterChain
    ├─ depends on → CompositionLayer
    │   ├─ TextureRenderer
    │   │   └─ VideoRenderer
    │   ├─ ImageRenderer (future)
    │   └─ TextRenderer (future)
    └─ depends on → InputState

FilterChain
    ├─ depends on → Filter
    │   ├─ ShaderFilter
    │   └─ ShaderEffect ⭐
    └─ depends on → ShaderConfig

ShaderLibrary
    ├─ loads → WGSL Shaders
    └─ creates → ShaderConfig
```

---

## 核心组件

### 1. CompositionEngine (组合引擎)

**定位**: SDK 的核心调度器，类似视频编辑软件的时间线

**职责**:
- 管理多个 CompositionLayer (如 After Effects 的时间线层)
- 按 z-order 排序并渲染所有层
- 应用全局后处理效果 (类似调整图层)
- 管理用户输入状态 (鼠标、键盘)
- 协调 WebGPU 资源生命周期

**API 示例**:
```cpp
CompositionEngine engine;

// 方式1: 内部管理窗口 (适合独立应用)
CeConfigure config = {.width = 1920, .height = 1080, .hwnd = hwnd};
engine.initialize(config);

// 方式2: 使用外部 device (适合集成到其他引擎)
engine.initialize(device, format, width, height);

// 层管理
size_t idx = engine.addLayer(std::make_unique<VideoRenderer>());
engine.getLayer(idx)->setEnabled(false);
engine.removeLayer(idx);

// 全局效果
engine.getGlobalFilterChain().addFilter(ShaderEffect::createBlur());

// 渲染循环
engine.update(deltaTime);
engine.render(outputView);
engine.present();  // 仅在内部管理窗口时使用
```

**设计模式**:
- **Composite Pattern**: 多层组合
- **Command Pattern**: 渲染命令封装
- **Observer Pattern**: 输入事件分发

---

### 2. CompositionLayer (组合层)

**层次结构**:
```cpp
CompositionLayer (抽象基类)
├── TextureRenderer (纹理渲染器)
│   ├── VideoRenderer (视频层)
│   │   ├── Planar Mode (2D 平面)
│   │   └── Panorama Mode (360° 全景)
│   └── ImageRenderer (图像层 - 未来)
├── TextRenderer (文字层 - 未来)
└── ShapeRenderer (形状层 - 未来)
```

**核心属性**:
- `layer`: z-order (0=背景, 数字越大越靠前)
- `enabled`: 可见性 (类似 AE 的眼睛图标)
- `name`: 层名称 (调试和 UI)
- `transform`: 位置和大小 (x, y, w, h)

**使用场景**:
```cpp
// Picture-in-Picture 示例
// 主视频 (全屏背景)
auto bgVideo = std::make_unique<VideoRenderer>();
bgVideo->setLayer(0);
bgVideo->setTransform(0, 0, 1, 1);

// 小窗叠加
auto pipVideo = std::make_unique<VideoRenderer>();
pipVideo->setLayer(1);
pipVideo->setTransform(0.7, 0.7, 0.25, 0.25);  // 右下角
```

---

### 3. VideoRenderer (视频渲染器)

**支持格式**:
- **NV12**: 硬件解码常用 (Y + UV planes)
- **I420**: YUV420P
- **RGBA**: 标准 RGBA

**渲染模式**:
- **Planar**: 标准 2D 渲染
- **Panorama**: 360° 球面投影 (VR/全景视频)

**D3D11 集成** (硬件加速):
```cpp
// FFmpeg → D3D11 → WebGPU 零拷贝
ID3D11Texture2D* srcTex = (ID3D11Texture2D*)frame->data[0];
int subIndex = (int)(intptr_t)frame->data[1];
videoRenderer->updateFrame(srcTex, subIndex);
```

**实现原理**:
1. 创建 D3D11 共享纹理 (SHARED_NTHANDLE)
2. CopySubresourceRegion 到共享纹理
3. ImportSharedTextureMemory 到 WebGPU
4. 创建 Plane0 (Y) 和 Plane1 (UV) 视图
5. 绑定到着色器

---

### 4. Filter (滤镜基类)

**职责**:
- 定义滤镜通用接口
- 管理 Pipeline、BindGroup、Uniform Buffer
- 提供参数更新机制

**继承层次**:
```
Filter (抽象基类)
├── ShaderFilter (基础着色器滤镜)
└── ShaderEffect (参数化效果) ⭐ 推荐
```

---

### 5. ShaderEffect (通用效果类) ⭐ 最重要

这是架构的核心创新！

**特点**:
- **无需为每个效果创建新类**
- **参数系统自动化**
- **支持运行时调整**
- **可序列化/反序列化**
- **支持交互式效果** (鼠标、时间)

**参数系统**:
```cpp
using ShaderParamValue = std::variant<
    float, int, bool,
    std::array<float, 2>,  // vec2
    std::array<float, 3>,  // vec3
    std::array<float, 4>   // vec4
>;

struct ShaderParam {
    std::string name;
    ShaderParamValue defaultValue;
    ShaderParamValue minValue;
    ShaderParamValue maxValue;
    std::string description;
};
```

**使用示例**:
```cpp
// 预设效果
auto colorAdjust = ShaderEffect::createColorAdjust();
colorAdjust->setParam("brightness", 0.2f);
colorAdjust->setParam("contrast", 1.3f);

auto blur = ShaderEffect::createBlur(5.0f);
auto vignette = ShaderEffect::createVignette();

// 交互式效果 (自动绑定 InputState)
auto spotlight = ShaderEffect::createMouseSpotlight();
spotlight->enableInputBinding(true);  // 自动绑定 iMouse, iTime 等
```

**内置效果预设**:
| 效果 | 工厂方法 | 参数 |
|------|----------|------|
| 颜色调整 | `createColorAdjust()` | brightness, contrast, saturation, hue |
| 高斯模糊 | `createBlur(radius)` | radius |
| 锐化 | `createSharpen(amount)` | amount |
| 暗角 | `createVignette()` | intensity, radius |
| 色差 | `createChromaticAberration()` | amount |
| 聚光灯 | `createMouseSpotlight()` | radius, intensity |
| 像素化 | `createPixelation()` | size |

---

### 6. FilterChain (滤镜链)

**架构**:
```
Input Texture
     ↓
┌────────────────┐
│  Filter Pass 1 │ → Intermediate Texture 1
└────────────────┘
     ↓
┌────────────────┐
│  Filter Pass 2 │ → Intermediate Texture 2
└────────────────┘
     ↓
┌────────────────┐
│  Filter Pass N │ → Output Texture
└────────────────┘
```

**自动资源管理**:
- Ping-pong 中间纹理
- 最小化纹理创建
- 自动绑定输入/输出

**使用示例**:
```cpp
FilterChain& chain = engine.getGlobalFilterChain();

// 按顺序添加效果
chain.addFilter(ShaderEffect::createColorAdjust());
chain.addFilter(ShaderEffect::createBlur(5.0f));
chain.addFilter(ShaderEffect::createVignette());

// 应用到输出
chain.apply(encoder, inputs, output, &inputState);
```

---

### 7. ShaderLibrary (着色器库)

**职责**:
- 从文件加载 WGSL 着色器
- 创建 ShaderConfig
- 统一管理着色器资源

**着色器类型**:
```cpp
enum class ShaderType {
    PlanarNV12,        // 平面 NV12
    PlanarRGBA,        // 平面 RGBA
    PanoramaNV12,      // 全景 NV12
    PanoramaRGBA,      // 全景 RGBA
    ColorAdjust,       // 颜色调整
    Blur,              // 模糊
    Sharpen,           // 锐化
    Vignette,          // 暗角
    // ...
};
```

**加载流程**:
```cpp
// 从文件加载
ShaderConfig config = ShaderLibrary::create(ShaderType::PanoramaNV12);
// 自动从 shaders/panorama_nv12.wgsl 加载

// 使用配置创建渲染器/效果
auto renderer = std::make_unique<TextureRenderer>(config);
```

---

### 8. InputState (输入状态管理)

**目的**: 为交互式效果提供统一的输入接口 (类似 Shadertoy)

**状态结构**:
```cpp
struct InputState {
    struct {
        float x, y;              // 归一化坐标 [0,1]
        float clickX, clickY;    // 最后点击位置
        bool leftButton;
        float wheelDelta;
    } mouse;

    struct {
        bool shift, ctrl, alt, space;
    } keyboard;

    struct {
        float elapsed;           // 总时间
        float delta;             // 帧间隔
        uint32_t frameCount;
    } time;

    struct {
        uint32_t width, height;
        float aspectRatio;
    } resolution;
};
```

**着色器自动绑定**:
```wgsl
struct Uniforms {
    iMouse: vec4f,       // (x, y, clickX, clickY)
    iTime: f32,
    iTimeDelta: f32,
    iFrame: u32,
    iResolution: vec3f,  // (width, height, aspect)
};
```

---

## 渲染流程

### 单帧渲染流程

```
1. update(deltaTime)
   ├── 更新 InputState (时间、帧数)
   ├── 遍历所有 Layer
   │   └── layer->update(deltaTime)
   └── InputState.resetPerFrameState()

2. render(outputView)
   ├── sortLayersByOrder()  // 按 z-order 排序
   ├── 创建 CommandEncoder
   ├
   ├── 确定渲染目标
   │   ├── if (globalFilterChain.isEmpty())
   │   │   └── target = outputView
   │   └── else
   │       └── target = intermediateTexture
   │
   ├── 主渲染 Pass
   │   ├── Clear 背景
   │   ├── for each layer (按 z-order)
   │   │   ├── if (!layer->isEnabled()) continue
   │   │   ├── if (has layerFilterChain)
   │   │   │   ├── render to intermediate
   │   │   │   ├── apply filterChain
   │   │   │   └── composite to main
   │   │   └── else
   │   │       └── render directly
   │   └── End Pass
   │
   ├── 全局后处理
   │   └── globalFilterChain.apply(input, output)
   │
   └── Submit CommandBuffer

3. present()
   └── surface.Present()
```

---

## 着色器系统

### WGSL 着色器结构

**全屏四边形顶点着色器**:
```wgsl
@vertex
fn vs_main(@builtin(vertex_index) vertex_index: u32) -> VertexOutput {
    // 生成全屏三角形
    let x = f32((vertex_index & 1u) << 2u) - 1.0;
    let y = f32((vertex_index & 2u) << 1u) - 1.0;

    var output: VertexOutput;
    output.position = vec4f(x, y, 0.0, 1.0);
    output.uv = vec2f((x + 1.0) * 0.5, (1.0 - y) * 0.5);
    return output;
}
```

**片段着色器模板**:
```wgsl
// Group 0: 纹理
@group(0) @binding(0) var inputTexture: texture_2d<f32>;
@group(0) @binding(1) var inputSampler: sampler;

// Group 1: Uniform 参数
struct Uniforms {
    brightness: f32,
    contrast: f32,
    saturation: f32,
    hue: f32,
};
@group(1) @binding(0) var<uniform> uniforms: Uniforms;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let color = textureSample(inputTexture, inputSampler, in.uv);
    // Apply effect...
    return result;
}
```

---

## 性能优化

### 1. GPU 资源管理
- **Dirty Flag**: 参数变化时才重建 BindGroup
- **资源池化**: Texture Pool, Buffer Pool
- **延迟创建**: 按需创建资源

### 2. 渲染优化
- **静态合批**: 合并相同材质的 Layer
- **Early-Z**: 前向后渲染不透明物体
- **中间纹理复用**: Ping-pong 缓冲

### 3. 多线程
- **解码与渲染分离**: 独立线程
- **锁粒度最小化**: 只保护数据传递
- **异步纹理上传**: (未来)

### 4. 内存优化
- **零拷贝传输**: D3D11 Shared Texture
- **RAII**: 自动资源管理
- **智能指针**: 引用计数

---

## 扩展性设计

### 自定义效果

```cpp
// 定义着色器
ShaderConfig myConfig;
myConfig.fragmentShaderSource = R"(
    @group(0) @binding(0) var mySampler: sampler;
    @group(0) @binding(1) var inputTexture: texture_2d<f32>;
    @group(1) @binding(0) var<uniform> params: vec2f;

    @fragment
    fn fs(input: VertexOutput) -> @location(0) vec4f {
        let color = textureSample(inputTexture, mySampler, input.uv);
        // 自定义处理...
        return result;
    }
)";

// 定义参数
std::vector<ShaderParam> params = {
    {"strength", 1.0f, 0.0f, 5.0f, "Effect strength"}
};

// 创建效果
auto effect = std::make_unique<ShaderEffect>("MyEffect", myConfig, params);
```

---

## 使用示例

### 完整示例 - Picture-in-Picture

```cpp
#include <clipengine/core/CompositionEngine.h>
#include <clipengine/layers/VideoRenderer.h>
#include <clipengine/effects/ShaderEffect.h>

// 1. 初始化引擎
CompositionEngine engine;
CeConfigure config = {.width = 1920, .height = 1080, .hwnd = hwnd};
engine.initialize(config);

// 2. 添加背景视频 (Layer 0)
auto bgVideo = std::make_unique<VideoRenderer>();
bgVideo->setLayer(0);
bgVideo->setTransform(0, 0, 1, 1);  // 全屏
bgVideo->setName("Background Video");
size_t bgIdx = engine.addLayer(std::move(bgVideo));

// 3. 添加 PiP 叠加视频 (Layer 1)
auto pipVideo = std::make_unique<VideoRenderer>();
pipVideo->setLayer(1);
pipVideo->setTransform(0.7, 0.7, 0.25, 0.25);  // 右下角 25%
pipVideo->setName("PiP Overlay");
size_t pipIdx = engine.addLayer(std::move(pipVideo));

// 4. 为 PiP 添加边框效果 (每层独立效果链)
FilterChain* pipFX = engine.getLayerFilterChain(pipIdx);
auto border = ShaderEffect::createVignette();
border->setParam("intensity", 0.8f);
pipFX->addFilter(std::move(border));

// 5. 添加全局后处理
auto colorGrading = ShaderEffect::createColorAdjust();
colorGrading->setParam("brightness", 0.1f);
colorGrading->setParam("contrast", 1.2f);
engine.getGlobalFilterChain().addFilter(std::move(colorGrading));

// 6. 渲染循环
while (running) {
    // 更新视频帧
    bgVideo->updateFrame(bgTexture, 0);
    pipVideo->updateFrame(pipTexture, 0);

    // 动态调整参数
    colorGrading->setParam("brightness", getBrightnessFromUI());

    engine.update(deltaTime);

    wgpu::SurfaceTexture surfaceTexture;
    surface.GetCurrentTexture(&surfaceTexture);
    wgpu::TextureView outputView = surfaceTexture.texture.CreateView();

    engine.render(outputView);
    engine.present();
}
```

### 360° 全景视频示例

```cpp
// 创建全景视频渲染器
auto panorama = std::make_unique<VideoRenderer>();
panorama->setRenderMode(VideoRenderer::RenderMode::Panorama);
panorama->setAspect(16.0f / 9.0f);
size_t panoIdx = engine.addLayer(std::move(panorama));

VideoRenderer* panoRenderer = static_cast<VideoRenderer*>(engine.getLayer(panoIdx));

// 交互式控制
while (running) {
    // 鼠标拖拽旋转
    panoRenderer->setRotation(yaw, pitch);

    // 滚轮缩放
    panoRenderer->setZoom(zoom);

    // 更新和渲染
    panoRenderer->updateFrame(texture, 0);
    engine.update(deltaTime);
    engine.render(outputView);
}
```

---

## 线程安全模型

### 解码线程 vs 渲染线程

```
解码线程 (Decoder Thread)
   ├── FFmpeg 解码 AVFrame
   ├── 获取 D3D11Texture2D
   └── [Lock] → 写入 frameData → [Unlock]

渲染线程 (Render Thread)
   ├── [Lock] → 读取 frameData → [Unlock]
   ├── updateFrame(texture, subIndex)  // 在锁外执行
   └── render()
```

**关键点**:
- **锁粒度最小化**: 只保护数据传递
- **GPU 操作在锁外**: 避免阻塞解码线程
- **使用 std::mutex**: 保证线程安全

**优化后的代码**:
```cpp
// 错误示例 - 锁内执行 GPU 操作
{
    std::lock_guard<std::mutex> lock(frameMutex_);
    if (frameData_.hasNewFrame) {
        videoRenderer->updateFrame(frameData_.texture, frameData_.subIndex);  // ❌
    }
}

// 正确示例 - 拷贝数据后释放锁
ID3D11Texture2D* currentTexture = nullptr;
int currentSubIndex = 0;
bool hasFrame = false;

{
    std::lock_guard<std::mutex> lock(frameMutex_);
    if (frameData_.hasNewFrame) {
        currentTexture = frameData_.texture;
        currentSubIndex = frameData_.subIndex;
        hasFrame = true;
        frameData_.hasNewFrame = false;
    }
}  // 锁释放

if (hasFrame) {
    videoRenderer->updateFrame(currentTexture, currentSubIndex);  // ✅
}
```

---

## 典型应用场景

### 1. 视频播放器
- 多种视频格式支持 (NV12, I420, RGBA)
- 硬件加速解码
- 实时颜色调整

### 2. 视频编辑器
- 多轨道时间线 (多层合成)
- 丰富的视觉效果
- Picture-in-Picture
- 实时预览

### 3. VR/全景应用
- 360° 视频播放
- 交互式旋转和缩放
- 等距柱状投影

### 4. 直播软件
- 多源合成
- 实时特效
- 低延迟渲染

---

## 未来规划

### 短期 (v1.1)
- [ ] ImageRenderer (静态图像)
- [ ] TextRenderer (文字渲染 + FreeType)
- [ ] 更多内置效果 (色度键、LUT、胶片颗粒)
- [ ] 关键帧动画系统

### 中期 (v1.5)
- [ ] 时间线编辑器
- [ ] 音频同步和混音
- [ ] GPU 编码 (H.264/H.265)
- [ ] 插件系统

### 长期 (v2.0)
- [ ] 移动平台支持 (iOS/Android)
- [ ] 云渲染
- [ ] AI 驱动的效果
- [ ] 实时协作编辑

---

## 依赖项

| 库 | 版本 | 用途 |
|---|---|---|
| **Dawn** | latest | WebGPU 实现 |
| **GLFW** | 3.x | 窗口管理 |
| **FFmpeg** | 6.x | 视频解码 |
| **spdlog** | 1.x | 日志系统 |
| **D3D11** | Windows SDK | 硬件加速 (Windows) |

---

## 构建系统

```bash
# 1. 构建 SDK
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# 2. 安装到 example/deps
cmake --install build --prefix example/deps/ClipEngineSDK-Release

# 3. 构建示例
cd example
cmake -B build
cmake --build build --config Release
```

---

## 总结

ClipEngine SDK 采用**现代化的图形 API (WebGPU)** 和**模块化的架构设计**，为视频渲染和后期处理提供了强大而灵活的解决方案。

### 核心优势

1. **高性能**
   - GPU 加速渲染
   - 零拷贝纹理传输
   - Ping-pong 中间纹理优化

2. **易扩展**
   - 数据驱动的着色器系统
   - ShaderEffect 通用效果框架
   - 插件化设计

3. **跨平台**
   - 基于 WebGPU 标准
   - 支持 Vulkan/D3D12/Metal backend
   - 未来支持移动端

4. **灵活**
   - 多层合成系统
   - 可堆叠的效果链
   - 实时参数调整

5. **现代**
   - C++20
   - RAII 资源管理
   - 智能指针

### 核心理念

**滤镜 = Shader + 参数**

这个简单而强大的理念贯穿整个架构设计，让开发者能够：
- 快速添加新效果 (只需编写 WGSL 着色器)
- 灵活调整参数 (运行时动态修改)
- 统一管理资源 (自动化的资源生命周期)

### 适用场景

✅ 视频播放器
✅ 视频编辑器
✅ 直播软件
✅ VR/AR 应用
✅ 实时渲染引擎
✅ 多媒体处理工具

---

**Remember**: 99% 的效果都可以用 `ShaderEffect` 实现，无需编写额外的 C++ 类！
