# ClipEngine 多 Renderer 架构

## 概述

ClipEngine 现在支持多个 renderer，每个 renderer 可以使用不同的 shader 进行渲染。Shader 不再硬编码在 renderer 中，而是通过 `ShaderConfig` 动态传入。

## 架构组件

### 1. ShaderConfig (ShaderConfig.h/cpp)

`ShaderConfig` 结构体封装了 shader 的所有配置信息：

- **Shader 源码**: vertex shader 和 fragment shader
- **绑定描述**: sampler、texture、buffer 的绑定配置
- **顶点布局**: 顶点属性和步长
- **管线设置**: 拓扑类型、深度模板等

### 2. ShaderPresets

提供预定义的 shader 配置：

- `createNV12VideoShader()` - NV12 视频格式 (Y + UV 平面)
- `createI420VideoShader()` - I420 视频格式 (Y + U + V 平面)
- `createRGBATextureShader()` - RGBA 纹理
- `createColorShader()` - 简单颜色 shader (用于测试)

### 3. TextureRenderer (TextureRenderer.h/cpp)

通用的纹理渲染器，接受任意 `ShaderConfig`：

```cpp
// 创建一个使用自定义 shader 的 renderer
auto shaderConfig = ShaderPresets::createRGBATextureShader();
auto renderer = std::make_unique<TextureRenderer>(shaderConfig);
renderer->setName("my_renderer");
renderer->setViewport(0, 0, 1, 1);
```

主要方法：
- `updateTextures(views)` - 更新纹理绑定
- `setViewport(x, y, w, h)` - 设置渲染视口 (0-1 归一化坐标)

### 4. VideoRenderer (VideoRenderer.h/cpp)

继承自 `TextureRenderer`，专门处理视频渲染：

```cpp
// 方式1: 使用默认 NV12 格式
auto videoRenderer = std::make_unique<VideoRenderer>();

// 方式2: 指定视频格式
auto videoRenderer = std::make_unique<VideoRenderer>(VideoFormat::I420);

// 方式3: 使用完全自定义的 shader
auto customConfig = ShaderPresets::createNV12VideoShader();
auto videoRenderer = std::make_unique<VideoRenderer>(customConfig);
```

特有功能：
- `updateFrame(texture, arrayIndex)` - 从 D3D11 纹理更新视频帧
- `setVideoFormat(format)` - 动态切换视频格式

## 使用示例

### 示例1: 单个视频 Renderer

```cpp
auto renderer = std::make_unique<VideoRenderer>();
renderer->setName("main_video");
renderer->setViewport(0, 0, 1, 1);  // 全屏
ce.addRenderer(std::move(renderer));

// 更新帧
CeRenderable* r = ce.getRendererByName("main_video");
((VideoRenderer*)r)->updateFrame(d3d11Texture, arrayIndex);
```

### 示例2: 多个 Renderer (画中画)

```cpp
// 主视频 (全屏)
auto mainVideo = std::make_unique<VideoRenderer>();
mainVideo->setName("main_video");
mainVideo->setViewport(0, 0, 1, 1);
mainVideo->setLayer(0);
ce.addRenderer(std::move(mainVideo));

// 小窗口视频 (右上角 1/4 大小)
auto pipVideo = std::make_unique<VideoRenderer>();
pipVideo->setName("pip_video");
pipVideo->setViewport(0.75f, 0.0f, 0.25f, 0.25f);  // 右上角
pipVideo->setLayer(1);  // 显示在主视频之上
ce.addRenderer(std::move(pipVideo));

// 字幕层 (底部)
auto subtitleRenderer = std::make_unique<TextureRenderer>(
    ShaderPresets::createRGBATextureShader()
);
subtitleRenderer->setName("subtitle");
subtitleRenderer->setViewport(0, 0.8f, 1, 0.2f);  // 底部 20%
subtitleRenderer->setLayer(2);  // 最上层
ce.addRenderer(std::move(subtitleRenderer));
```

### 示例3: 自定义 Shader

```cpp
// 创建自定义 shader 配置
ShaderConfig customConfig;
customConfig.name = "My Custom Shader";
customConfig.vertexShaderSource = R"(
    @vertex
    fn vs(@location(0) pos : vec2f) -> @builtin(position) vec4f {
        return vec4f(pos, 0.0, 1.0);
    }
)";
customConfig.fragmentShaderSource = R"(
    @fragment
    fn fs() -> @location(0) vec4f {
        return vec4f(1.0, 0.0, 0.0, 1.0);  // 红色
    }
)";
// ... 配置 bindings 和 vertex attributes ...

auto renderer = std::make_unique<TextureRenderer>(customConfig);
ce.addRenderer(std::move(renderer));
```

## Layer 系统

Renderer 通过 `setLayer(int)` 设置渲染顺序：
- Layer 值越小，越先渲染（在底层）
- Layer 值越大，越后渲染（在上层）
- 相同 layer 的 renderer 按照添加顺序渲染

```cpp
backgroundRenderer->setLayer(0);   // 背景
videoRenderer->setLayer(1);        // 视频
overlayRenderer->setLayer(2);      // 叠加层
subtitleRenderer->setLayer(3);     // 字幕
```

## 动态控制

可以在运行时动态控制 renderer：

```cpp
// 启用/禁用
renderer->setEnabled(false);  // 隐藏
renderer->setEnabled(true);   // 显示

// 修改视口
renderer->setViewport(0.5f, 0.5f, 0.5f, 0.5f);  // 移动到右下角

// 修改 layer
renderer->setLayer(5);  // 移到最上层

// 切换视频格式
((VideoRenderer*)renderer)->setVideoFormat(VideoFormat::RGBA);
```

## 架构优势

1. **灵活性**: Shader 不再硬编码，可以动态创建和修改
2. **可扩展性**: 轻松添加新的 renderer 类型和 shader
3. **多层渲染**: 支持多个 renderer 同时渲染，实现复杂的合成效果
4. **性能**: 每个 renderer 独立管理资源，互不干扰
5. **易用性**: 提供预设的 shader，同时支持完全自定义

## 注意事项

1. **Viewport 坐标系**: 使用 0-1 归一化坐标，(0, 0) 为左下角，(1, 1) 为右上角
2. **Shader 兼容性**: Shader 必须使用 WGSL (WebGPU Shading Language)
3. **纹理绑定**: `TextureRenderer::updateTextures()` 的纹理数量必须与 shader 的绑定配置匹配
4. **资源管理**: Renderer 会自动管理内部资源的生命周期

## 未来扩展

可以轻松添加更多 renderer 类型：
- `AudioVisualizerRenderer` - 音频可视化
- `ParticleRenderer` - 粒子效果
- `TransitionRenderer` - 转场效果
- `FilterRenderer` - 滤镜效果（模糊、锐化等）
