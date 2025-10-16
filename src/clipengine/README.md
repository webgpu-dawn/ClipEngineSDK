# ClipEngine

专业级音视频编辑引擎核心库

## 概述

ClipEngine 是一个高性能的音视频编辑引擎，提供完整的编解码、渲染、特效等功能。

## 架构设计

```
clipengine/
├── core/           # 核心基础类
│   ├── ClipEngine.h        # 引擎主类
│   ├── GPUContext.h        # GPU上下文管理
│   ├── RHIHelper.h         # 渲染硬件接口辅助
│   └── Types.h             # 基础类型定义
│
├── render/         # 渲染模块
│   ├── RenderEngine.h      # 渲染引擎
│   ├── Renderer.h          # 渲染器基类
│   ├── VideoRenderer.h     # 视频渲染器
│   └── Layer.h             # 图层系统
│
├── codec/          # 编解码模块（预留）
│   ├── Decoder.h           # 解码器接口
│   ├── Encoder.h           # 编码器接口
│   └── CodecFactory.h      # 编解码工厂
│
├── shader/         # 着色器模块
│   ├── ShaderLibrary.h     # 着色器库
│   ├── video.wgsl          # 视频着色器
│   └── effect.wgsl         # 特效着色器
│
└── util/           # 工具类
    ├── Math.h              # 数学工具
    └── Logger.h            # 日志系统
```

## 模块说明

### Core - 核心模块
引擎的核心基础设施，包括：
- **ClipEngine**: 引擎主类，统一的入口点
- **GPUContext**: GPU上下文管理（WebGPU/Dawn）
- **RHIHelper**: 简化的渲染资源创建工具
- **Types**: 通用类型定义

### Render - 渲染模块（v1.0实现）
负责所有图像渲染相关功能：
- **RenderEngine**: 渲染引擎，管理渲染流程
- **Renderer**: 渲染器基类接口
- **VideoRenderer**: 视频帧渲染（支持NV12 GPU零拷贝）
- **Layer**: 图层系统（多轨道、画中画）

### Codec - 编解码模块（预留）
音视频编解码功能：
- **Decoder**: 解码器接口（H264/H265/VP9等）
- **Encoder**: 编码器接口
- **CodecFactory**: 编解码器工厂

### Shader - 着色器模块
WGSL着色器资源：
- **ShaderLibrary**: 着色器管理和加载
- **video.wgsl**: 视频渲染着色器（YUV→RGB）
- **effect.wgsl**: 特效着色器（预留）

### Util - 工具模块
通用工具类：
- **Math**: 数学计算（矩阵、向量）
- **Logger**: 日志系统

## 版本规划

### v1.0 - 图像渲染引擎
- ✅ GPU上下文管理
- ✅ 视频帧渲染（NV12格式）
- ✅ 多层渲染系统
- ✅ Viewport控制
- ✅ RHI辅助工具

### v1.1 - 编解码支持（规划中）
- ⏳ FFmpeg解码器集成
- ⏳ 硬件解码（D3D11/NVDEC）
- ⏳ 编码器接口

### v2.0 - 音频支持（规划中）
- ⏳ 音频解码
- ⏳ 音频渲染
- ⏳ 音视频同步

### v3.0 - 特效系统（规划中）
- ⏳ 滤镜效果
- ⏳ 转场特效
- ⏳ 时间线编辑

## 快速开始

```cpp
#include "clipengine/core/ClipEngine.h"
#include "clipengine/render/VideoRenderer.h"

// 创建引擎
ClipEngine::EngineConfig config;
config.width = 1920;
config.height = 1080;

ClipEngine::Engine engine;
engine.initialize(config);

// 创建视频渲染器
auto videoRenderer = engine.createVideoRenderer();
videoRenderer->setViewport(0.0f, 0.0f, 1.0f, 1.0f);

// 渲染视频帧
videoRenderer->updateFrame(texture, arrayIndex);
engine.render();
```

## 依赖

- **WebGPU/Dawn** - GPU渲染
- **GLFW** - 窗口管理
- **FFmpeg** - 编解码（可选）
- **C++17** - 编译器要求

## 许可证

MIT License

---

**ClipEngine** - Professional Video Editing Engine
