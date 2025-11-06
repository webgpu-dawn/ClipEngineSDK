# Application.cpp 迁移指南

> **从旧 API 迁移到新 Layer + Effect API 的详细对比**

## 📋 概述

本文档展示了 `samples/Application.cpp` 从旧架构迁移到新架构的详细步骤。

## 🔄 头文件更改

### 旧代码（Application.h）

```cpp
#include <clipengine/core/CompositionEngine.h>
#include <clipengine/layers/VideoRenderer.h>
#include <clipengine/layers/TextureRenderer.h>
#include <clipengine/effects/ShaderEffect.h>

class Application {
private:
    CompositionEngine engine_;
    VideoRenderer* videoRenderer_ = nullptr;
    TextureRenderer* imageRenderer_ = nullptr;
    ShaderEffect* colorAdjust_ = nullptr;
};
```

### 新代码（推荐）

```cpp
#include <clipengine/layers/VideoLayer.h>
#include <clipengine/layers/ImageLayer.h>
#include <clipengine/effects/ColorAdjustEffect.h>
#include <clipengine/compositor/Compositor.h>

class Application {
private:
    // 新架构组件
    std::shared_ptr<VideoLayer> videoLayer_;
    std::shared_ptr<ImageLayer> imageLayer_;
    std::shared_ptr<ColorAdjustEffect> colorEffect_;

    LayerStack layerStack_;
    std::unique_ptr<Compositor> compositor_;

    // 如果需要保持向后兼容，可以保留
    CompositionEngine engine_;  // 旧系统
};
```

---

## 🎬 视频图层创建

### 旧代码（Application.cpp:85-100）

```cpp
void Application::setupScene()
{
    // 创建 VideoRenderer
    auto video = std::make_unique<VideoRenderer>();
    video->setTransform(0.0f, 0.0f, 1.0f, 1.0f);  // 归一化坐标
    video->setLayer(0);
    video->setName("Main Video Layer");
    video->setAspect((float)width_ / (float)height_);
    size_t videoIdx = engine_.addLayer(std::move(video));

    videoRenderer_ = static_cast<VideoRenderer*>(engine_.getLayer(videoIdx));

    // 设置渲染模式
    if (videoRenderer_) {
        videoRenderer_->setRenderMode(VideoRenderer::RenderMode::Panorama);
    }
}
```

### 新代码（推荐）

```cpp
void Application::setupScene()
{
    // 创建 VideoLayer
    videoLayer_ = std::make_shared<VideoLayer>();
    videoLayer_->initialize(engine_.getDevice(), engine_.getFormat());

    // 使用 Transform2D（像素坐标）
    videoLayer_->transform.position = {960.0f, 540.0f};  // 居中 (1920/2, 1080/2)
    videoLayer_->transform.scale = {1.0f, 1.0f};
    videoLayer_->transform.rotation = 0.0f;
    videoLayer_->transform.opacity = 1.0f;

    // 设置属性
    videoLayer_->setName("Main Video Layer");
    videoLayer_->setZOrder(0);  // 替代 setLayer

    // 设置投影模式（全景）
    videoLayer_->setProjection(VideoProjection::Equirectangular);
}
```

**关键变化**:
- ✅ `std::make_unique` → `std::make_shared`（Layer 使用共享指针）
- ✅ `setTransform(x, y, w, h)` → `transform.position/scale`（像素坐标）
- ✅ `setLayer(0)` → `setZOrder(0)`（更清晰）
- ✅ `setRenderMode(Panorama)` → `setProjection(Equirectangular)`
- ✅ 不再需要 `addLayer` + `getLayer` 的繁琐操作

---

## 🖼️ 图片图层创建

### 旧代码（Application.cpp:102-112）

```cpp
// 创建图片叠加层（使用 VideoRenderer 伪装成图片层）
auto imageLayer = std::make_unique<VideoRenderer>(VideoFormat::RGBA);
imageLayer->setTransform(0.0f, 0.0f, 1.0f, 1.0f);  // 全屏
imageLayer->setLayer(1);  // 在视频上层
imageLayer->setName("Image Overlay Layer");
imageLayer->setEnabled(true);  // 初始隐藏
size_t imageIdx = engine_.addLayer(std::move(imageLayer));

imageRenderer_ = static_cast<TextureRenderer*>(engine_.getLayer(imageIdx));
```

### 新代码（推荐）

```cpp
// 创建 ImageLayer（专门的图片层）
imageLayer_ = std::make_shared<ImageLayer>();
imageLayer_->initialize(engine_.getDevice(), engine_.getFormat());

// Transform（左上角，20% 大小）
imageLayer_->transform.position = {100.0f, 100.0f};  // 左上角
imageLayer_->transform.scale = {0.2f, 0.2f};         // 缩小到 20%
imageLayer_->transform.opacity = 0.8f;               // 80% 不透明度

// 属性
imageLayer_->setName("Image Overlay Layer");
imageLayer_->setZOrder(1);      // 在视频上层
imageLayer_->setVisible(false);  // 初始隐藏（替代 setEnabled）
```

**关键变化**:
- ✅ `VideoRenderer(RGBA)` → `ImageLayer`（专门的类型）
- ✅ `setEnabled()` → `setVisible()`（更清晰）
- ✅ Transform 使用像素坐标，更直观
- ✅ 不再需要类型转换（`static_cast<TextureRenderer*>`）

---

## 🎨 颜色调整效果

### 旧代码（Application.cpp:56-69）

```cpp
// 使用全局 FilterChain
auto colorAdjust = ShaderEffect::createColorAdjust();

// 字符串参数设置
colorAdjust->setParam("brightness", 0.0f);   // 0 = 正常
colorAdjust->setParam("contrast", 1.0f);
colorAdjust->setParam("saturation", 1.0f);
colorAdjust->setParam("exposure", 0.0f);
colorAdjust->setParam("gain", 1.0f);
colorAdjust->setParam("hue", 0.0f);

// 保存指针，添加到全局 FilterChain
colorAdjust_ = colorAdjust.get();
engine_.getGlobalFilterChain().addFilter(std::move(colorAdjust));
```

### 新代码（推荐）

```cpp
// 创建 ColorAdjustEffect
colorEffect_ = std::make_shared<ColorAdjustEffect>();
colorEffect_->initialize(engine_.getDevice(), engine_.getFormat());

// 方式1: 类型安全的方法（推荐）
colorEffect_->setBrightness(1.0f);   // 1.0 = 正常（注意：新 API 的默认值不同）
colorEffect_->setContrast(1.0f);
colorEffect_->setSaturation(1.0f);
colorEffect_->setExposure(0.0f);
colorEffect_->setGain(1.0f);

// 方式2: 通用参数接口（也可以）
// colorEffect_->setParameter("brightness", 1.0f);

// 添加到 VideoLayer（不是全局）
videoLayer_->addEffect(colorEffect_);
```

**关键变化**:
- ✅ `ShaderEffect::createColorAdjust()` → `std::make_shared<ColorAdjustEffect>()`
- ✅ `setParam("name", value)` → `setBrightness(value)`（类型安全）
- ✅ `engine_.getGlobalFilterChain().addFilter()` → `layer->addEffect()`（每个 Layer 独立）
- ⚠️ **注意**: 旧 API 的 brightness 默认值是 0，新 API 是 1.0（映射关系不同）

---

## ⚡ 参数调整（键盘控制）

### 旧代码（Application.cpp:403-415）

```cpp
// Brightness 调整
case GLFW_KEY_Q:
    brightness_ += 0.05f;
    if (brightness_ > 1.0f) brightness_ = 1.0f;
    if (colorAdjust_) colorAdjust_->setParam("brightness", brightness_);
    std::cout << "Brightness: " << brightness_ << std::endl;
    break;

case GLFW_KEY_W:
    brightness_ -= 0.05f;
    if (brightness_ < -1.0f) brightness_ = -1.0f;
    if (colorAdjust_) colorAdjust_->setParam("brightness", brightness_);
    std::cout << "Brightness: " << brightness_ << std::endl;
    break;
```

### 新代码（推荐）

```cpp
// Brightness 调整（新 API）
case GLFW_KEY_Q:
    brightness_ += 0.1f;
    if (brightness_ > 2.0f) brightness_ = 2.0f;  // 新 API: 0.0 - 2.0
    if (colorEffect_) colorEffect_->setBrightness(brightness_);
    std::cout << "Brightness: " << brightness_ << std::endl;
    break;

case GLFW_KEY_W:
    brightness_ -= 0.1f;
    if (brightness_ < 0.0f) brightness_ = 0.0f;  // 新 API: 0.0 - 2.0
    if (colorEffect_) colorEffect_->setBrightness(brightness_);
    std::cout << "Brightness: " << brightness_ << std::endl;
    break;
```

**关键变化**:
- ✅ `colorAdjust_->setParam()` → `colorEffect_->setBrightness()`
- ⚠️ **参数范围变化**:
  - 旧: `-1.0 - 1.0` (0 = 正常)
  - 新: `0.0 - 2.0` (1.0 = 正常)

---

## 🔄 重置参数

### 旧代码（Application.cpp:463-477）

```cpp
case GLFW_KEY_R:
    brightness_ = 0.0f;    // 旧 API: 0 = 正常
    contrast_ = 1.0f;
    saturation_ = 1.0f;
    exposure_ = 0.0f;
    gain_ = 1.0f;
    if (colorAdjust_) {
        colorAdjust_->setParam("brightness", brightness_);
        colorAdjust_->setParam("contrast", contrast_);
        colorAdjust_->setParam("saturation", saturation_);
        colorAdjust_->setParam("exposure", exposure_);
        colorAdjust_->setParam("gain", gain_);
    }
    std::cout << "Reset all color adjustments" << std::endl;
    break;
```

### 新代码（推荐）

```cpp
case GLFW_KEY_R:
    brightness_ = 1.0f;    // 新 API: 1.0 = 正常
    contrast_ = 1.0f;
    saturation_ = 1.0f;
    exposure_ = 0.0f;
    gain_ = 1.0f;
    if (colorEffect_) {
        colorEffect_->setBrightness(brightness_);
        colorEffect_->setContrast(contrast_);
        colorEffect_->setSaturation(saturation_);
        colorEffect_->setExposure(exposure_);
        colorEffect_->setGain(gain_);
    }
    std::cout << "Reset all color adjustments" << std::endl;
    break;
```

**关键变化**:
- ✅ 使用类型安全的方法
- ⚠️ **brightness 默认值**: `0.0f` → `1.0f`

---

## 🎥 视频帧更新

### 旧代码（Application.cpp:174-180）

```cpp
void Application::updateVideoFrame()
{
    if (videoSource_->hasNewFrame() && videoRenderer_) {
        auto frame = videoSource_->getLatestFrame();
        videoRenderer_->updateFrame(frame.texture, frame.subIndex);
    }
}
```

### 新代码（推荐）

```cpp
void Application::updateVideoFrame()
{
    if (videoSource_->hasNewFrame() && videoLayer_) {
        auto frame = videoSource_->getLatestFrame();
        videoLayer_->updateFrame(frame.texture, frame.subIndex);
    }
}
```

**关键变化**:
- ✅ `videoRenderer_` → `videoLayer_`

---

## 🖼️ 图片加载

### 旧代码（Application.cpp:508-542）

```cpp
void Application::loadImageTexture(const std::string& imagePath)
{
    if (!imageRenderer_) {
        std::cerr << "Image renderer not initialized" << std::endl;
        return;
    }

    // 加载图片
    ComPtr<ID3D11Texture2D> imageTexture;
    uint32_t imageWidth, imageHeight;

    if (!ImageLoader::loadImage(imagePath, imageTexture, imageWidth, imageHeight)) {
        std::cerr << "Failed to load image texture" << std::endl;
        return;
    }

    // 使用 VideoRenderer 更新纹理
    auto videoRenderer = dynamic_cast<VideoRenderer*>(imageRenderer_);
    if (videoRenderer) {
        videoRenderer->setVideoFormat(VideoFormat::RGBA);
        videoRenderer->setTransform(0, 0, 0.2, 0.2);  // 归一化坐标
        videoRenderer->updateFrame(imageTexture.Get(), 0);
    }

    imageRenderer_->setEnabled(true);
}
```

### 新代码（推荐）

```cpp
void Application::loadImageTexture(const std::string& imagePath)
{
    if (!imageLayer_) {
        std::cerr << "Image layer not initialized" << std::endl;
        return;
    }

    // 使用 ImageLayer 的 loadImage 方法
    if (!imageLayer_->loadImage(imagePath)) {
        std::cerr << "Failed to load image" << std::endl;
        return;
    }

    // 调整 Transform（像素坐标）
    imageLayer_->transform.position = {100.0f, 100.0f};  // 左上角
    imageLayer_->transform.scale = {0.2f, 0.2f};         // 20% 大小

    // 显示图层
    imageLayer_->setVisible(true);

    std::cout << "Image loaded and displayed" << std::endl;
}
```

**关键变化**:
- ✅ `imageRenderer_` → `imageLayer_`
- ✅ `loadImage()` 内置到 ImageLayer
- ✅ 不再需要 `setVideoFormat(RGBA)` 的 hack
- ✅ `setEnabled()` → `setVisible()`
- ✅ Transform 使用像素坐标

---

## 🎮 投影模式切换

### 旧代码（Application.cpp:382-400）

```cpp
case GLFW_KEY_1:
    switchRenderMode(VideoRenderer::RenderMode::Planar, 0.0f, 0.0f, 1.0f);
    std::cout << "Switched to Planar mode" << std::endl;
    break;

case GLFW_KEY_2:
    switchRenderMode(VideoRenderer::RenderMode::Panorama, 0.0f, 0.0f, 1.0f);
    std::cout << "Switched to Panorama mode" << std::endl;
    break;

void Application::switchRenderMode(VideoRenderer::RenderMode mode, float yaw, float pitch, float zoom)
{
    if (!videoRenderer_) return;

    videoRenderer_->setRenderMode(mode);
    yaw_ = yaw;
    pitch_ = pitch;
    zoom_ = zoom;
    videoRenderer_->setRotation(yaw_, pitch_);
    videoRenderer_->setZoom(zoom_);
}
```

### 新代码（推荐）

```cpp
case GLFW_KEY_1:
    switchProjection(VideoProjection::Planar, 0.0f, 0.0f, 1.0f);
    std::cout << "Switched to Planar mode" << std::endl;
    break;

case GLFW_KEY_2:
    switchProjection(VideoProjection::Equirectangular, 0.0f, 0.0f, 1.0f);
    std::cout << "Switched to Equirectangular (360°) mode" << std::endl;
    break;

void Application::switchProjection(VideoProjection projection, float yaw, float pitch, float zoom)
{
    if (!videoLayer_) return;

    videoLayer_->setProjection(projection);
    yaw_ = yaw;
    pitch_ = pitch;
    zoom_ = zoom;
    videoLayer_->setRotation(yaw_, pitch_);
    videoLayer_->setZoom(zoom_);
}
```

**关键变化**:
- ✅ `RenderMode` → `VideoProjection`
- ✅ `setRenderMode()` → `setProjection()`
- ✅ `videoRenderer_` → `videoLayer_`

---

## 📊 完整对比表

| 功能 | 旧 API | 新 API |
|------|--------|--------|
| **视频图层** | `VideoRenderer` | `VideoLayer` |
| **图片图层** | `VideoRenderer(RGBA)` | `ImageLayer` |
| **创建方式** | `std::make_unique` | `std::make_shared` |
| **Transform** | `setTransform(x,y,w,h)` 归一化 | `transform.position/scale` 像素坐标 |
| **图层顺序** | `setLayer(n)` | `setZOrder(n)` |
| **可见性** | `setEnabled(bool)` | `setVisible(bool)` |
| **投影模式** | `setRenderMode(RenderMode)` | `setProjection(VideoProjection)` |
| **效果类型** | `ShaderEffect` | `ColorAdjustEffect` |
| **效果创建** | `ShaderEffect::createColorAdjust()` | `std::make_shared<ColorAdjustEffect>()` |
| **参数设置** | `setParam("name", value)` 字符串 | `setBrightness(value)` 类型安全 |
| **效果管理** | `getGlobalFilterChain().addFilter()` | `layer->addEffect()` 每层独立 |
| **Brightness 范围** | `-1.0 - 1.0` (0=正常) | `0.0 - 2.0` (1.0=正常) |

---

## ⚠️ 迁移注意事项

### 1. Brightness 参数映射

旧 API 和新 API 的 brightness 参数范围不同：

```cpp
// 旧 API: -1.0 到 1.0，0 = 正常
float oldBrightness = 0.0f;  // 正常

// 新 API: 0.0 到 2.0，1.0 = 正常
float newBrightness = 1.0f;  // 正常

// 转换公式:
newBrightness = oldBrightness + 1.0f;
oldBrightness = newBrightness - 1.0f;
```

### 2. Transform 坐标系统

```cpp
// 旧 API: 归一化坐标 (0.0 - 1.0)
setTransform(0.0f, 0.0f, 1.0f, 1.0f);  // 全屏

// 新 API: 像素坐标
transform.position = {960, 540};  // 中心 (1920/2, 1080/2)
transform.scale = {1.0f, 1.0f};   // 100%
```

### 3. 效果作用范围

```cpp
// 旧 API: 全局 FilterChain，所有 Layer 共享
engine_.getGlobalFilterChain().addFilter(effect);

// 新 API: 每个 Layer 独立的 Effect 链
videoLayer->addEffect(effect);   // 只影响 videoLayer
imageLayer->addEffect(effect2);  // 只影响 imageLayer
```

---

## ✅ 迁移检查清单

- [ ] 替换头文件 include
- [ ] 替换 `VideoRenderer` → `VideoLayer`
- [ ] 替换 `TextureRenderer` → `ImageLayer`
- [ ] 更新 Transform 使用方式（归一化坐标 → 像素坐标）
- [ ] 替换 `setLayer()` → `setZOrder()`
- [ ] 替换 `setEnabled()` → `setVisible()`
- [ ] 替换 `ShaderEffect` → `ColorAdjustEffect`
- [ ] 替换 `setParam()` → `setBrightness()` 等类型安全方法
- [ ] 替换 `getGlobalFilterChain()` → `layer->addEffect()`
- [ ] 更新 brightness 参数范围（0 → 1.0）
- [ ] 替换 `setRenderMode()` → `setProjection()`
- [ ] 测试所有功能

---

## 📚 相关文档

- [API_MIGRATION_GUIDE.md](./API_MIGRATION_GUIDE.md) - 通用 API 迁移指南
- [LAYER_EFFECT_USAGE.md](./LAYER_EFFECT_USAGE.md) - Layer + Effect 详细用法
- [samples/new_api_example/](../samples/new_api_example/) - 新 API 完整示例

---

**最后更新**: 2025-11-06
**核心理念**: Layer 可以添加 Effect，Effect 可以调整参数 ✨
