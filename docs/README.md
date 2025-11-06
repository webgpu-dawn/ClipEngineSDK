# ClipEngine 文档中心

> 📚 完整的架构设计、API 参考和渲染管线文档

---

## ⚡ Layer + Effect 新架构 (2025-11-06 更新)

### 🎯 快速入口

| 文档 | 说明 | 适合人群 |
|------|------|---------|
| [NEW_API_SUMMARY.md](NEW_API_SUMMARY.md) | **新架构总结** ⭐ | 所有人，从这里开始！ |
| [API_MIGRATION_GUIDE.md](API_MIGRATION_GUIDE.md) | 通用迁移指南 | 开发者迁移旧代码 |
| [APPLICATION_MIGRATION.md](APPLICATION_MIGRATION.md) | Application.cpp 专用迁移 | 详细逐行对比 |
| [LAYER_EFFECT_USAGE.md](LAYER_EFFECT_USAGE.md) | Layer + Effect 详细用法 | 完整 API 说明 |
| [samples/new_api_example/](../samples/new_api_example/) | 完整示例程序 | 可运行的代码 |

### 核心概念

```cpp
// ✨ 新架构：Layer 可以添加 Effect，Effect 可以调整参数

// 1. 创建 Layer
auto video = std::make_shared<VideoLayer>();
video->transform.position = {960, 540};  // 像素坐标

// 2. 添加 Effect
auto effect = std::make_shared<ColorAdjustEffect>();
effect->setBrightness(1.2f);  // 类型安全
video->addEffect(effect);     // 每个 Layer 独立

// 3. 合成渲染
LayerStack stack;
stack.addLayer(video);
Compositor compositor;
compositor.render(stack, time);
```

**从旧 API 迁移**: VideoRenderer → VideoLayer, addFilter → addEffect

---

## 🚀 快速导航

### 新手入门

| 文档 | 说明 | 适合人群 |
|------|------|---------|
| [NEW_SRC_SUMMARY.md](NEW_SRC_SUMMARY.md) | 新架构快速总结 | 所有人 ⭐ |
| [images/new_src_architecture_simple.svg](images/new_src_architecture_simple.svg) | 简化架构图 | 快速预览 |

### 架构设计

| 文档 | 说明 | 适合人群 |
|------|------|---------|
| [NEW_SRC_ARCHITECTURE_DIAGRAMS.md](NEW_SRC_ARCHITECTURE_DIAGRAMS.md) | 完整架构图汇总 + 设计理念 | 架构师、技术负责人 |
| [ARCHITECTURE.md](ARCHITECTURE.md) | 旧架构设计文档 | 参考对比 |

### 渲染管线

| 文档 | 说明 | 适合人群 |
|------|------|---------|
| [RENDER_PIPELINE_IO.md](RENDER_PIPELINE_IO.md) | 渲染管线输入输出详解 ⭐ | 图形开发者 |
| [images/render_pipeline_io.svg](images/render_pipeline_io.svg) | 渲染管线 I/O 图表 | 可视化学习 |

### API 参考

| 文档 | 说明 | 适合人群 |
|------|------|---------|
| [API_REFERENCE.md](API_REFERENCE.md) | API 参考手册 | 应用开发者 |

---

## 📊 架构图总览

### 1. 简化流程图（推荐入门）
![简化架构](images/new_src_architecture_simple.svg)

**包含内容**:
- ✅ 4 层架构流程
- ✅ 关键设计理念
- ✅ Adobe 概念对齐
- ✅ 技术栈

**适用场景**: 快速理解整体架构

---

### 2. 渲染管线输入输出图（推荐开发者）
![渲染管线 I/O](images/render_pipeline_io.svg)

**包含内容**:
- ✅ 每一层的输入格式
- ✅ 处理流程
- ✅ 输出格式
- ✅ 数据类型说明

**适用场景**: 理解渲染流程，开发图层

---

### 3. 完整系统架构图
![系统架构](images/new_src_architecture.svg)

**包含内容**:
- ✅ 项目管理层
- ✅ 图层系统
- ✅ 渲染引擎
- ✅ 导出系统

---

### 4. 模块依赖关系图
![模块依赖](images/new_src_module_dependencies.svg)

**包含内容**:
- ✅ 核心引擎层
- ✅ 业务模块层
- ✅ 外部依赖

---

### 5. 渲染数据流图
![数据流](images/new_src_dataflow.svg)

**包含内容**:
- ✅ 4 阶段渲染流程
- ✅ 详细的处理步骤

---

### 6. 类层次结构图
![类层次](images/new_src_class_hierarchy.svg)

**包含内容**:
- ✅ 继承关系
- ✅ 组合关系
- ✅ 类成员方法

---

## 🎯 按需求查找

### 我想了解...

#### 📖 **整体架构设计**
👉 查看 [NEW_SRC_ARCHITECTURE_DIAGRAMS.md](NEW_SRC_ARCHITECTURE_DIAGRAMS.md)

包含：
- 架构设计理念（为什么这样设计？）
- Adobe 概念对齐
- 完整数据流说明
- 设计决策 FAQ

---

#### 🎨 **图层的渲染流程**
👉 查看 [RENDER_PIPELINE_IO.md](RENDER_PIPELINE_IO.md)

包含：
- ImageLayer 输入输出
- TextLayer 输入输出
- ShapeLayer 输入输出
- 合成器工作原理
- 完整代码示例

---

#### 📦 **如何使用 API**
👉 查看 [API_REFERENCE.md](API_REFERENCE.md)

包含：
- Project API
- Layer API
- Export API
- 代码示例

---

#### 🔍 **模块间的依赖关系**
👉 查看 [模块依赖关系图](images/new_src_module_dependencies.svg)

---

## 🎨 与 Adobe 概念对齐

| Adobe 概念 | ClipEngine | 状态 |
|-----------|-----------|------|
| Project | `Project` | ✅ |
| Composition | `Composition` | ✅ |
| Layer | `CompositionLayer` | ✅ |
| Image Layer | `ImageLayer` | ✅ |
| Text Layer | `TextLayer` | ✅ |
| Shape Layer | `ShapeLayer` | ✅ |
| Timeline | `Timeline` | ✅ |
| Keyframe | `Animation` | ✅ |
| Export | `VideoExporter` / `ImageExporter` | ✅ |
| Effect | `Effect` | ⏳ Phase 2 |
| Color Management | `ColorManager` | ⏳ Phase 3 |

详细对照请查看 [NEW_SRC_ARCHITECTURE_DIAGRAMS.md](NEW_SRC_ARCHITECTURE_DIAGRAMS.md#-与-adobe-概念对齐)

---

## 🔄 渲染流程速查

### 完整流程

```
1. 图层渲染
   ├─ ImageLayer: 文件路径 → GPU TextureView
   ├─ TextLayer:  文字字符串 → GPU TextureView
   └─ ShapeLayer: 矢量路径 → GPU TextureView

2. 变换处理
   └─ Transform Matrix → 应用到顶点

3. 混合合成
   └─ 多个 TextureView → 逐层混合 → FrameBuffer

4. 导出
   ├─ GPU → CPU 回读
   ├─ RGBA → YUV 转换
   └─ FFmpeg 编码 → 视频文件
```

详细说明请查看 [RENDER_PIPELINE_IO.md](RENDER_PIPELINE_IO.md)

---

## 📁 目录结构

```
new_src/
├── clipengine/
│   ├── project/              # 项目管理
│   │   ├── Project.h         # 项目容器
│   │   ├── AssetManager.h    # 资源管理
│   │   ├── Timeline.h        # 时间轴
│   │   └── Animation.h       # 关键帧动画
│   │
│   ├── layers/               # 图层系统
│   │   ├── ImageLayer.h      # 图片图层
│   │   ├── TextLayer.h       # 文字图层
│   │   └── ShapeLayer.h      # 形状图层
│   │
│   └── export/               # 导出系统
│       ├── VideoExporter.h   # 视频导出
│       └── ImageExporter.h   # 图片导出
│
└── docs/
    └── README.md            # 本文件
```

---

## 🤔 常见问题

### Q: 每种图层的输出格式是什么？
**A**: 所有图层都输出 `wgpu::TextureView`（GPU 纹理视图），格式为 RGBA8。详见 [RENDER_PIPELINE_IO.md](RENDER_PIPELINE_IO.md)

### Q: 数据在 CPU 和 GPU 之间如何传输？
**A**:
- **上传**: CPU (uint8_t*) → `wgpu::Queue.writeTexture()` → GPU (wgpu::Texture)
- **下载**: GPU (wgpu::Texture) → `copyTextureToBuffer()` → CPU (uint8_t*)

详见 [RENDER_PIPELINE_IO.md](RENDER_PIPELINE_IO.md#阶段-5-读回与导出-readback--export)

### Q: 为什么要分 ImageLayer/TextLayer/ShapeLayer？
**A**: 参考 Adobe 设计，职责分离。每种图层专注自己的领域，代码更简洁、易维护。详见 [NEW_SRC_ARCHITECTURE_DIAGRAMS.md](NEW_SRC_ARCHITECTURE_DIAGRAMS.md#2️⃣-图层系统---基于继承的多态设计)

### Q: 渲染过程中数据都在 GPU 吗？
**A**: 是的！所有渲染处理都在 GPU，只在导出时才回读到 CPU。这是性能的关键。详见 [RENDER_PIPELINE_IO.md](RENDER_PIPELINE_IO.md#⚡-性能关键点)

---

## 📚 外部资源

- [WebGPU 规范](https://www.w3.org/TR/webgpu/)
- [wgpu-native API](https://wgpu-native.github.io/wgpu-native/)
- [stb_image 文档](https://github.com/nothings/stb)
- [FFmpeg 文档](https://ffmpeg.org/documentation.html)
- [Adobe After Effects 概念](https://helpx.adobe.com/after-effects/using/composition-basics.html)

---

## 🔗 快速链接

| 链接 | 说明 |
|------|------|
| [../README.md](../README.md) | 项目主文档 |
| [../new_src/docs/README.md](../new_src/docs/README.md) | 新架构快速导航 |

---

**文档版本**: 1.0
**创建日期**: 2025-11-04
**维护者**: ClipEngine Team
