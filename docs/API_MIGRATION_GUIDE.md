# API Migration Guide - 从旧架构迁移到新架构

> 🔄 **从 VideoRenderer/Filter 迁移到 Layer/Effect**

---

## 📋 概念对应关系

| 旧架构 | 新架构 | 说明 |
|--------|--------|------|
| `VideoRenderer` | `VideoLayer` | 视频图层 |
| `TextureRenderer` | `ImageLayer` | 图片图层 |
| `Filter` | `Effect` | 效果（支持参数调整）|
| `FilterChain` | Layer 内置 Effect 链 | 每个 Layer 自带 Effect 链 |
| `addFilter()` | `addEffect()` | 添加效果 |
| `setParam()` | `setParameter()` | 设置参数 |
| `CompositionEngine` | `Compositor` | 合成器 |

---

## 🔄 迁移步骤

### Step 1: 替换头文件

**旧代码**:
```cpp
#include <clipengine/layers/VideoRenderer.h>
#include <clipengine/layers/TextureRenderer.h>
#include <clipengine/effects/ShaderEffect.h>
#include <clipengine/effects/FilterChain.h>
```

**新代码**:
```cpp
#include <clipengine/layers/VideoLayer.h>
#include <clipengine/layers/ImageLayer.h>
#include <clipengine/layers/SolidLayer.h>
#include <clipengine/effects/Effect.h>
#include <clipengine/effects/ColorAdjustEffect.h>
#include <clipengine/compositor/Compositor.h>
```

---

### Step 2: 替换 VideoRenderer → VideoLayer

**旧代码**:
```cpp
// 创建 VideoRenderer
auto video = std::make_unique<VideoRenderer>();
video->setTransform(0.0f, 0.0f, 1.0f, 1.0f);
video->setLayer(0);
video->setName("Main Video");
video->setRenderMode(VideoRenderer::RenderMode::Panorama);
size_t idx = engine_.addLayer(std::move(video));
videoRenderer_ = static_cast<VideoRenderer*>(engine_.getLayer(idx));
```

**新代码**:
```cpp
// 创建 VideoLayer（Layer 概念）
auto video = std::make_shared<VideoLayer>();
video->initialize(device, format);
video->loadVideo("video.mp4");

// 使用 Transform2D 设置位置
video->transform.position = {960, 540};  // 居中
video->transform.scale = {1.0f, 1.0f};
video->transform.opacity = 1.0f;

// 设置投影模式
video->setProjection(VideoProjection::Equirectangular);  // 360° 全景
video->setZOrder(0);  // 图层顺序
video->setName("Main Video");
```

---

### Step 3: 替换 Filter → Effect

**旧代码**:
```cpp
// 使用 ShaderEffect 和 FilterChain
auto colorAdjust = ShaderEffect::createColorAdjust();
colorAdjust->setParam("brightness", 0.0f);
colorAdjust->setParam("contrast", 1.0f);
colorAdjust->setParam("saturation", 1.0f);

ShaderEffect* colorAdjust_ = colorAdjust.get();
engine_.getGlobalFilterChain().addFilter(std::move(colorAdjust));
```

**新代码**:
```cpp
// 使用 Effect（Layer 概念，每个 Layer 自己管理 Effect）
auto colorAdjust = std::make_shared<ColorAdjustEffect>();
colorAdjust->initialize(device, format);

// 方式1：使用通用参数接口
colorAdjust->setParameter("brightness", 1.2f);
colorAdjust->setParameter("contrast", 1.1f);
colorAdjust->setParameter("saturation", 0.9f);

// 方式2：使用类型安全的方法（推荐）
colorAdjust->setBrightness(1.2f);
colorAdjust->setContrast(1.1f);
colorAdjust->setSaturation(0.9f);

// 添加到 Layer（而不是全局 FilterChain）
video->addEffect(colorAdjust);
```

---

### Step 4: 替换 TextureRenderer → ImageLayer

**旧代码**:
```cpp
// 使用 VideoRenderer(RGBA) 显示图片
auto imageLayer = std::make_unique<VideoRenderer>(VideoFormat::RGBA);
imageLayer->setTransform(0.0f, 0.0f, 1.0f, 1.0f);
imageLayer->setLayer(1);
imageLayer->setName("Image Overlay");
imageLayer->setEnabled(false);
size_t imageIdx = engine_.addLayer(std::move(imageLayer));
imageRenderer_ = static_cast<TextureRenderer*>(engine_.getLayer(imageIdx));
```

**新代码**:
```cpp
// 使用 ImageLayer（Layer 概念）
auto image = std::make_shared<ImageLayer>();
image->initialize(device, format);
image->loadImage("overlay.png");

// Transform
image->transform.position = {100, 100};  // 左上角
image->transform.scale = {0.3f, 0.3f};   // 缩小到 30%
image->transform.opacity = 0.8f;          // 80% 不透明度

image->setZOrder(1);  // 在视频上层
image->setName("Image Overlay");
image->setVisible(true);
```

---

### Step 5: 替换 CompositionEngine → Compositor

**旧代码**:
```cpp
CompositionEngine engine_;
engine_.initialize(config);
size_t idx = engine_.addLayer(std::move(layer));
auto layer = engine_.getLayer(idx);
engine_.render();
```

**新代码**:
```cpp
// 使用 LayerStack 和 Compositor
LayerStack stack;
stack.addLayer(videoLayer);
stack.addLayer(imageLayer);

Compositor compositor;
compositor.initialize(device, format, 1920, 1080);
auto result = compositor.render(stack, currentTime);
```

---

## 📝 完整示例对比

### 旧架构示例

```cpp
#include <clipengine/core/CompositionEngine.h>
#include <clipengine/layers/VideoRenderer.h>
#include <clipengine/effects/ShaderEffect.h>

int main() {
    // 1. 初始化引擎
    CompositionEngine engine;
    DeviceConfig config = { ... };
    engine.initialize(config);

    // 2. 创建视频渲染器
    auto video = std::make_unique<VideoRenderer>();
    video->setTransform(0.0f, 0.0f, 1.0f, 1.0f);
    video->setLayer(0);
    video->setRenderMode(VideoRenderer::RenderMode::Panorama);
    size_t idx = engine.addLayer(std::move(video));

    // 3. 添加全局滤镜
    auto colorAdjust = ShaderEffect::createColorAdjust();
    colorAdjust->setParam("brightness", 0.2f);
    engine.getGlobalFilterChain().addFilter(std::move(colorAdjust));

    // 4. 渲染
    engine.render();
}
```

### 新架构示例

```cpp
#include <clipengine/layers/VideoLayer.h>
#include <clipengine/effects/ColorAdjustEffect.h>
#include <clipengine/compositor/Compositor.h>

int main() {
    // 1. 初始化设备
    wgpu::Device device = ...;
    wgpu::TextureFormat format = ...;

    // 2. 创建 VideoLayer（Layer 概念）
    auto video = std::make_shared<VideoLayer>();
    video->initialize(device, format);
    video->loadVideo("video.mp4");

    // 3. 设置 Transform
    video->transform.position = {960, 540};
    video->transform.scale = {1.0f, 1.0f};

    // 4. 设置投影模式
    video->setProjection(VideoProjection::Equirectangular);

    // 5. 添加 Effect（Layer 自己的 Effect，不是全局）
    auto colorAdjust = std::make_shared<ColorAdjustEffect>();
    colorAdjust->initialize(device, format);
    colorAdjust->setBrightness(1.2f);  // 类型安全
    video->addEffect(colorAdjust);     // addEffect（不是 addFilter）

    // 6. 创建 LayerStack
    LayerStack stack;
    stack.addLayer(video);

    // 7. 使用 Compositor 合成
    Compositor compositor;
    compositor.initialize(device, format, 1920, 1080);
    auto result = compositor.render(stack, 0.0f);
}
```

---

## 🎯 关键区别

### 1. Layer 概念更清晰

**旧**: VideoRenderer, TextureRenderer (概念混乱)
**新**: VideoLayer, ImageLayer, SolidLayer (清晰的 Layer 概念)

### 2. Transform 内置

**旧**: `setTransform(x, y, w, h)` (归一化坐标)
**新**: `layer->transform.position/rotation/scale` (像素坐标 + 完整变换)

### 3. Effect 独立管理

**旧**: 全局 FilterChain，所有 Layer 共享
**新**: 每个 Layer 独立的 Effect 链

**旧**:
```cpp
engine.getGlobalFilterChain().addFilter(effect);  // 全局
```

**新**:
```cpp
layer->addEffect(effect);  // 每个 Layer 独立
```

### 4. 参数调整更友好

**旧**:
```cpp
effect->setParam("brightness", 0.2f);  // 字符串，易出错
```

**新**:
```cpp
effect->setParameter("brightness", 1.2f);  // 通用接口
effect->setBrightness(1.2f);                // 类型安全（推荐）
```

### 5. Z-Order 更明确

**旧**:
```cpp
layer->setLayer(0);  // Layer number
```

**新**:
```cpp
layer->setZOrder(0);  // 更清晰的命名
```

---

## 🔄 渐进式迁移策略

### Phase 1: 保持兼容（当前阶段）

```cpp
// 继续使用旧 API
VideoRenderer* video = ...;
engine.addLayer(...);
```

### Phase 2: 混合使用

```cpp
// 部分使用新 API
auto video = std::make_shared<VideoLayer>();
video->transform.position = {960, 540};  // 新 Transform
// 但仍然添加到旧的 CompositionEngine
```

### Phase 3: 完全迁移

```cpp
// 完全使用新 API
LayerStack stack;
stack.addLayer(videoLayer);
Compositor compositor;
compositor.render(stack, time);
```

---

## 💡 迁移检查清单

### 代码层面

- [ ] 替换 `VideoRenderer` → `VideoLayer`
- [ ] 替换 `TextureRenderer` → `ImageLayer`
- [ ] 替换 `addFilter()` → `addEffect()`
- [ ] 替换 `setParam()` → `setParameter()` 或类型安全方法
- [ ] 替换 `setLayer()` → `setZOrder()`
- [ ] 替换 `CompositionEngine` → `LayerStack + Compositor`
- [ ] 更新 Transform 使用方式

### 头文件

- [ ] 更新 include 路径
- [ ] 移除旧的 `effects/Filter.h`
- [ ] 添加新的 `effects/Effect.h`
- [ ] 添加 `compositor/Compositor.h`
- [ ] 添加 `transform/Transform2D.h`

### 概念理解

- [ ] 理解 Layer 概念
- [ ] 理解每个 Layer 独立管理 Effect
- [ ] 理解 Transform2D 的使用
- [ ] 理解 Compositor 的作用

---

## 📚 相关文档

- [LAYER_EFFECT_USAGE.md](./LAYER_EFFECT_USAGE.md) - Layer + Effect 详细用法
- [SIMPLE_API_GUIDE.md](./SIMPLE_API_GUIDE.md) - 简化 API 指南
- [ARCHITECTURE_REFACTOR.md](./ARCHITECTURE_REFACTOR.md) - 架构重构说明

---

**最后更新**: 2025-11-06
**核心理念**: Layer 可以添加 Effect，Effect 可以调整参数 ✨
