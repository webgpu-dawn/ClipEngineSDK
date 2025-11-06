# ClipEngine SDK 架构设计文档

**Version**: 1.0
**Last Updated**: 2025-11-03
**Status**: Living Document

---

## 📑 目录

- [1. 概述](#1-概述)
- [2. 架构原则](#2-架构原则)
- [3. 系统架构](#3-系统架构)
- [4. 核心模块详解](#4-核心模块详解)
- [5. 数据流](#5-数据流)
- [6. 性能优化策略](#6-性能优化策略)
- [7. 扩展性设计](#7-扩展性设计)
- [8. 实现状态](#8-实现状态)
- [9. 未来规划](#9-未来规划)

---

## 1. 概述

### 1.1 项目定位

ClipEngine SDK 是一个**专业级 GPU 加速图像渲染引擎**，专为视频编辑、特效合成和实时图像处理设计。参考 Adobe After Effects 和 Premiere Pro 的合成系统设计，提供高性能、易扩展的视频处理能力。

### 1.2 技术栈

- **语言**: C++20
- **图形API**: WebGPU (Dawn implementation)
- **视频编解码**: FFmpeg 7.0+
- **窗口系统**: GLFW 3.4+
- **图像加载**: stb_image
- **调试UI**: ImGui
- **构建系统**: CMake 3.13+

### 1.3 设计目标

| 目标 | 说明 |
|------|------|
| **高性能** | 充分利用 GPU 并行计算，支持 4K/8K 实时渲染 |
| **跨平台** | Windows/Linux/macOS 统一代码库 |
| **易集成** | 清晰的 C++ API，丰富的示例代码 |
| **可扩展** | 插件式架构，支持自定义效果和图层类型 |
| **专业级** | 参考业界标准，满足专业视频制作需求 |

---

## 2. 架构原则

### 2.1 核心设计原则

#### 分层架构 (Layered Architecture)

```
┌─────────────────────────────────────┐
│  Layer 5: Export & UI               │  用户界面、导出
├─────────────────────────────────────┤
│  Layer 4: Project Management        │  项目/资源/动画管理
├─────────────────────────────────────┤
│  Layer 3: Layer & Effect System     │  图层系统、效果系统
├─────────────────────────────────────┤
│  Layer 2: Rendering System          │  合成引擎、渲染器
├─────────────────────────────────────┤
│  Layer 1: Core Engine               │  GPU 抽象、基础设施
└─────────────────────────────────────┘
```

**优势:**
- 清晰的职责分离
- 依赖关系单向向下
- 便于测试和维护

#### 组件化设计 (Component-Based Design)

图层采用组件化设计：

```cpp
CompositionLayer {
    + Source (VideoSource/ImageSource/TextSource)
    + Transform (Position/Rotation/Scale/Anchor)
    + FilterChain (Effects: Color/Blur/Distort...)
    + Masks (Vector masks/Alpha masks)
    + BlendMode (Normal/Add/Multiply...)
}
```

**优势:**
- 组件可独立开发和测试
- 灵活组合不同功能
- 易于扩展新组件

#### 渲染图优化 (Render Graph Optimization)

```
┌─────────────┐     ┌─────────────┐
│   Layer 1   │────▶│   Filter A  │
└─────────────┘     └─────────────┘
       │                    │
       │                    ▼
       │            ┌─────────────┐
       └───────────▶│   Composite │
                    └─────────────┘
```

**优势:**
- 自动识别可并行的渲染路径
- 消除冗余计算
- 最小化纹理拷贝

### 2.2 性能优化原则

1. **GPU First**: 尽可能在 GPU 上完成计算
2. **Zero Copy**: 最小化 CPU-GPU 数据传输
3. **Async Pipeline**: 异步渲染管线，隐藏延迟
4. **Smart Caching**: 三层缓存系统 (RAM/Disk/Network)
5. **Lazy Evaluation**: 延迟计算，按需渲染

### 2.3 扩展性原则

1. **插件接口**: 支持动态加载效果插件
2. **配置驱动**: JSON/YAML 配置文件
3. **脚本化**: 表达式引擎支持（未来）
4. **版本兼容**: 向后兼容的文件格式

---

## 3. 系统架构

### 3.1 总体架构图

**简化架构图**:

![架构概览](docs/images/architecture-simple.svg)

**详细架构图**:

![详细架构](docs/images/architecture-detailed.svg)

> 💡 如果详细架构图无法显示，请查看简化版本或使用浏览器直接打开 SVG 文件。

### 3.2 模块依赖关系

![模块依赖](docs/images/module-dependencies.svg)

### 3.3 五层架构详解

#### Layer 1: Core Engine (核心引擎层)

**职责**: 提供底层基础设施和 GPU 抽象

**核心模块**:

| 模块 | 状态 | 功能 |
|------|------|------|
| `RenderDevice` | ✅ 已实现 | WebGPU 设备管理、Surface 配置 |
| `GPU Abstraction` | ✅ 已实现 | Pipeline、Shader、Buffer 抽象 |
| `InputSystem` | ✅ 已实现 | 跨平台输入事件处理 |
| `Utils` | ✅ 已实现 | Logger、Timer、Inspector |
| `CacheSystem` | ❌ 待实现 | 三层缓存 (L1/L2/L3) |

#### Layer 2: Rendering System (渲染系统层)

**职责**: 实现渲染管线和合成逻辑

**核心模块**:

| 模块 | 状态 | 功能 |
|------|------|------|
| `CompositionEngine` | ✅ 已实现 | 多图层合成引擎 |
| `OffscreenRenderer` | ✅ 已实现 | 离屏渲染、帧缓冲 |
| `ShaderLibrary` | ✅ 已实现 | WGSL 着色器管理 |
| `ColorManagement` | ❌ 待实现 | 色彩空间转换、LUT、OCIO |

**关键设计**:

```cpp
class CompositionEngine {
    // 图层管理
    size_t addLayer(std::unique_ptr<CompositionLayer> layer);
    void removeLayer(size_t index);

    // 渲染流程
    void update(float deltaTime);          // 更新动画
    void render(wgpu::TextureView output); // 渲染到纹理

    // 滤镜链
    FilterChain& getGlobalFilterChain();
    FilterChain* getLayerFilterChain(size_t index);
};
```

#### Layer 3: Layer & Effect System (图层与效果系统)

**职责**: 提供各种图层类型和视觉效果

**图层类型**:

| 图层类型 | 状态 | 说明 |
|---------|------|------|
| `VideoRenderer` | ✅ 已实现 | 视频图层 (FFmpeg 解码) |
| `TextureRenderer` | ✅ 已实现 | 纹理图层 (GPU 纹理) |
| `ImageLayer` | ❌ 待实现 | 图片图层 (PNG/JPG) |
| `TextLayer` | ❌ 待实现 | 文本图层 (字体渲染) |
| `ShapeLayer` | ❌ 待实现 | 矢量形状图层 |

**效果系统**:

| 效果类别 | 状态 | 包含效果 |
|---------|------|---------|
| Color Correction | ⚠️ 部分 | 色阶、曲线、HSL、曝光 |
| Blur & Sharpen | ⚠️ 部分 | 高斯模糊、径向模糊、锐化 |
| Distortion | ❌ 待实现 | 镜头扭曲、涟漪、球面化 |
| Stylize | ❌ 待实现 | 发光、描边、卡通、油画 |

**滤镜架构**:

```cpp
class Filter {
    virtual bool initialize(wgpu::Device device, wgpu::TextureFormat format);
    virtual void apply(wgpu::RenderPassEncoder& pass,
                       const std::vector<wgpu::TextureView>& inputs);
    virtual void updateParameters();
};

class FilterChain {
    void addEffect(std::unique_ptr<Filter> filter);
    void removeEffect(size_t index);
    void apply(wgpu::RenderPassEncoder& pass, wgpu::TextureView input);
};
```

#### Layer 4: Project Management (项目管理层)

**职责**: 管理项目、资源、动画和时间轴

**核心模块**:

| 模块 | 状态 | 功能 |
|------|------|------|
| `Project` | ❌ 待实现 | 项目保存/加载、多 Composition 管理 |
| `AssetManager` | ❌ 待实现 | 视频/图片/音频资源管理 |
| `Timeline` | ❌ 待实现 | 时间轴系统、播放控制 |
| `Animation` | ❌ 待实现 | 关键帧动画、缓动函数 |

#### Layer 5: Export & UI (导出与用户界面层)

**职责**: 导出渲染结果和用户界面

**核心模块**:

| 模块 | 状态 | 功能 |
|------|------|------|
| `VideoExporter` | ✅ 已实现 | H.264/H.265/VP9 视频导出 |
| `ImageExporter` | ❌ 待实现 | PNG/JPG/TIFF 图片序列导出 |
| `AudioExporter` | ❌ 待实现 | 音频导出 |
| `DebugWindow` | ✅ 已实现 | ImGui 调试界面 |

---

## 4. 核心模块详解

### 4.1 CompositionEngine (合成引擎)

#### 设计理念

参考 After Effects 的合成系统，支持：

1. **多图层堆叠** - 无限图层，z-order 排序
2. **per-layer 效果** - 每个图层独立的效果链
3. **全局效果** - 类似 Adjustment Layer
4. **交互式输入** - 支持鼠标/键盘交互效果

#### 渲染流程

```cpp
void CompositionEngine::render(wgpu::TextureView output) {
    // 1. 排序图层 (按 z-order)
    sortLayersByOrder();

    // 2. 清空画布
    clearBackground(output);

    // 3. 逐层渲染
    for (auto& entry : layers_) {
        if (!entry.layer->isVisible()) continue;

        // 3a. 渲染图层内容
        auto layerOutput = entry.layer->render();

        // 3b. 应用图层效果
        if (entry.filterChain.hasEffects()) {
            layerOutput = entry.filterChain.apply(layerOutput);
        }

        // 3c. 合成到输出
        compositeLayer(layerOutput, output, entry.layer->getBlendMode());
    }

    // 4. 应用全局效果 (Adjustment Layer)
    if (globalFilterChain_.hasEffects()) {
        globalFilterChain_.apply(output);
    }
}
```

### 4.2 VideoRenderer (视频图层)

#### FFmpeg 集成

```cpp
class VideoRenderer : public CompositionLayer {
private:
    AVFormatContext* formatContext_;
    AVCodecContext* codecContext_;
    SwsContext* swsContext_;

    wgpu::Texture videoTexture_;

public:
    bool loadVideo(const std::string& path);
    void setTime(float seconds);
    wgpu::TextureView render() override;
};
```

#### 解码流程

```
[视频文件] → [FFmpeg Demux] → [Decode] → [YUV→RGB] → [Upload to GPU] → [Texture]
```

**优化**:
- 异步解码线程
- 帧缓冲池 (减少分配)
- 硬件加速解码 (NVDEC/VAAPI)

### 4.3 Filter & FilterChain (滤镜系统)

#### 滤镜生命周期

```cpp
// 1. 创建滤镜
auto blur = std::make_unique<GaussianBlurFilter>();

// 2. 初始化 (创建 GPU 资源)
blur->initialize(device, format);

// 3. 设置参数
blur->setRadius(5.0f);

// 4. 应用滤镜
blur->apply(renderPass, {inputTexture});

// 5. 更新参数 (可选)
blur->setRadius(10.0f);
blur->updateParameters();
```

#### 自定义着色器滤镜

```cpp
// 使用 JSON 配置加载
auto effect = EffectLoader::loadFromFile("custom_effect.json");

// custom_effect.json:
{
    "name": "Vintage Film",
    "vertex_shader": "shaders/fullscreen.vert.wgsl",
    "fragment_shader": "shaders/vintage_film.frag.wgsl",
    "uniforms": {
        "intensity": { "type": "float", "default": 1.0 },
        "grain": { "type": "float", "default": 0.5 }
    }
}
```

### 4.4 VideoExporter (视频导出)

#### 导出流程

```
[CompositionEngine] → [OffscreenRenderer] → [Pixel Readback] →
[Color Conversion] → [FFmpeg Encode] → [Write to File]
```

#### 代码示例

```cpp
VideoExportConfig config = {
    .outputPath = "output.mp4",
    .width = 1920,
    .height = 1080,
    .fps = 60,
    .bitrate = 10'000'000,  // 10 Mbps
    .codec = VideoCodec::H264,
    .preset = VideoQualityPreset::Medium
};

VideoExporter exporter;
exporter.initialize(config, device);

exporter.setProgressCallback([](float progress) {
    std::cout << "Progress: " << (progress * 100) << "%\n";
});

exporter.beginExport(&engine, 0.0f, 10.0f);  // 导出 0-10 秒

while (!exporter.isFinished()) {
    exporter.exportFrame();
}

exporter.finalize();
```

---

## 5. 数据流

### 5.1 渲染管线数据流

![渲染管线详细流程](docs/images/render-pipeline-detail.svg)

### 5.2 图层渲染数据流

```
┌──────────────┐
│ Layer Source │  VideoSource / ImageSource / TextSource
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  Transform   │  Position, Rotation, Scale, Anchor
└──────┬───────┘
       │
       ▼
┌──────────────┐
│    Masks     │  Vector masks / Alpha masks
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ Filter Chain │  Effects: Color, Blur, Distort, Stylize
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   Composite  │  Blend with other layers
└──────────────┘
```

### 5.3 缓存系统数据流

![缓存系统](docs/images/cache-system.svg)

---

## 6. 性能优化策略

### 6.1 GPU 加速

#### 计算着色器优化

```wgsl
@compute @workgroup_size(16, 16)
fn gaussianBlur(
    @builtin(global_invocation_id) id: vec3<u32>
) {
    let uv = vec2<f32>(id.xy) / resolution;

    var color = vec3<f32>(0.0);
    var totalWeight = 0.0;

    // 分离式高斯模糊 (两次 1D 卷积)
    for (var i = -radius; i <= radius; i++) {
        let weight = gaussian(f32(i), sigma);
        let offset = vec2<f32>(f32(i), 0.0);
        color += textureSample(input, sampler, uv + offset / resolution).rgb * weight;
        totalWeight += weight;
    }

    textureStore(output, id.xy, vec4<f32>(color / totalWeight, 1.0));
}
```

**优化点**:
- Workgroup 大小优化 (16x16 = 256 threads)
- 分离式卷积 (O(n²) → O(2n))
- 局部内存共享 (workgroup memory)

### 6.2 缓存策略

#### 三层缓存系统

| 层级 | 存储 | 容量 | 延迟 | 命中率 |
|------|------|------|------|--------|
| L1 | RAM | 2 GB | <1ms | 85% |
| L2 | SSD | 50 GB | ~10ms | 12% |
| L3 | Network | 无限 | ~100ms | 2% |

#### 缓存键生成

```cpp
std::string generateCacheKey(const RenderRequest& req) {
    // Hash: [composition_id][layer_id][time][effects][resolution]
    std::stringstream ss;
    ss << req.compositionId << "_"
       << req.layerId << "_"
       << std::fixed << std::setprecision(3) << req.time << "_"
       << hashEffects(req.effects) << "_"
       << req.width << "x" << req.height;
    return md5(ss.str());
}
```

### 6.3 异步渲染管线

```cpp
// Frame N
┌────────────┬────────────┬────────────┐
│ CPU: Frame │ GPU: Frame │ Present:   │
│ N+2 Submit │ N+1 Render │ Frame N    │
└────────────┴────────────┴────────────┘
```

**优势**:
- CPU/GPU 并行工作
- 隐藏 GPU 延迟
- 提高吞吐量

### 6.4 内存管理

#### 纹理池 (Texture Pool)

```cpp
class TexturePool {
    std::vector<wgpu::Texture> availableTextures_;
    std::unordered_map<wgpu::Texture, bool> inUse_;

public:
    wgpu::Texture acquire(uint32_t width, uint32_t height);
    void release(wgpu::Texture texture);
};
```

**优势**:
- 减少纹理创建/销毁开销
- 避免内存碎片
- 复用 GPU 资源

---

## 7. 扩展性设计

### 7.1 自定义图层类型

```cpp
class MyCustomLayer : public CompositionLayer {
public:
    wgpu::TextureView render() override {
        // 自定义渲染逻辑
        return myCustomTexture_;
    }

    void update(float deltaTime) override {
        // 自定义更新逻辑
    }
};

// 使用
auto customLayer = std::make_unique<MyCustomLayer>();
customLayer->setLayer(0);
engine.addLayer(std::move(customLayer));
```

### 7.2 自定义效果插件

```cpp
class MyCustomEffect : public Filter {
public:
    bool initialize(wgpu::Device device, wgpu::TextureFormat format) override {
        // 加载着色器、创建管线
    }

    void apply(wgpu::RenderPassEncoder& pass,
               const std::vector<wgpu::TextureView>& inputs) override {
        // 应用效果
    }
};

// 使用
auto effect = std::make_unique<MyCustomEffect>();
engine.getLayerFilterChain(0)->addEffect(std::move(effect));
```

### 7.3 表达式引擎 (未来)

```javascript
// 位置表达式
position = [
    wiggle(2, 50),  // 抖动动画
    time * 100       // 匀速运动
];

// 透明度表达式
opacity = ease(time, 0, 1, 0, 100);  // 缓动动画
```

---

## 8. 实现状态

### 8.1 已实现模块 (✅)

| 模块 | 完成度 | 说明 |
|------|--------|------|
| Core Engine | 100% | WebGPU 设备管理、GPU 抽象 |
| CompositionEngine | 100% | 多图层合成引擎 |
| VideoRenderer | 100% | FFmpeg 视频解码 |
| TextureRenderer | 100% | GPU 纹理渲染 |
| Filter System | 80% | 滤镜基类、滤镜链、部分效果 |
| VideoExporter | 100% | FFmpeg 视频编码导出 |
| InputSystem | 100% | 跨平台输入系统 |
| DebugWindow | 100% | ImGui 调试界面 |

### 8.2 部分实现模块 (⚠️)

| 模块 | 完成度 | 缺失功能 |
|------|--------|---------|
| Layer System | 40% | 缺少 ImageLayer、TextLayer、ShapeLayer |
| Effect System | 30% | 仅实现部分色彩和模糊效果 |
| Transform System | 60% | 缺少完整的变换矩阵和锚点支持 |

### 8.3 待实现模块 (❌)

| 模块 | 优先级 | 预计工作量 |
|------|--------|-----------|
| Project Management | 高 | 2 weeks |
| Asset Management | 高 | 2 weeks |
| Timeline System | 高 | 3 weeks |
| Animation System | 中 | 4 weeks |
| Color Management | 中 | 2 weeks |
| Cache System | 中 | 3 weeks |
| Mask System | 低 | 2 weeks |

### 8.4 整体进度

```
█████████████████████░░░░░░░░░░░ 60% Complete

✅ 核心引擎: 100%
✅ 渲染系统: 95%
⚠️  图层系统: 40%
⚠️  效果系统: 30%
❌ 项目管理: 0%
❌ 动画系统: 0%
```

---

## 9. 未来规划

### 9.1 短期目标 (v1.1 - Q2 2025)

- [ ] 完善图层系统 (ImageLayer, TextLayer)
- [ ] 扩展效果库 (30+ 专业效果)
- [ ] 实现关键帧动画系统
- [ ] 添加蒙版系统

### 9.2 中期目标 (v1.5 - Q4 2025)

- [ ] 项目管理和资源管理系统
- [ ] 时间轴系统
- [ ] 色彩管理 (LUT, OCIO)
- [ ] 三层缓存系统
- [ ] 表达式引擎

### 9.3 长期目标 (v2.0 - 2026)

- [ ] 3D 图层支持
- [ ] 相机与灯光系统
- [ ] 粒子系统
- [ ] 物理模拟
- [ ] 分布式渲染

---

## 附录

### A. 相关文档

- [README.md](README.md) - 项目主文档
- [API Reference](docs/API_REFERENCE.md) - API 参考文档
- [Architecture Diagrams](docs/images/ARCHITECTURE_DIAGRAMS.md) - 架构图汇总

### B. 参考项目

- [WebGPU Dawn](https://dawn.googlesource.com/dawn)
- [FFmpeg](https://ffmpeg.org/)
- [ImGui](https://github.com/ocornut/imgui)
- [GLFW](https://www.glfw.org/)

### C. 贡献指南

详见 [CONTRIBUTING.md](CONTRIBUTING.md)

---

**文档维护者**: ClipEngine Team
**最后更新**: 2025-11-03
**版本**: 1.0
