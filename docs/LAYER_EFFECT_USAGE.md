# Layer + Effect 架构使用指南

> 🎨 **Layer 可以添加 Effect，Effect 可以调整参数**

---

## 📚 核心概念

### 1. Layer（图层）
图层是可渲染的视觉元素，每个图层可以：
- 包含内容（视频、图片、纯色等）
- 应用 Transform（位置、旋转、缩放）
- 添加 Effect（效果）

### 2. Effect（效果）
Effect 是应用于图层的视觉效果，每个 Effect 可以：
- 有多个可调整的参数
- 按顺序应用（Effect Chain）
- 独立启用/禁用

---

## 🎯 基本用法

### 创建 Layer 并添加 Effect

```cpp
#include <clipengine/layers/VideoLayer.h>
#include <clipengine/effects/ColorAdjustEffect.h>

// 1. 创建视频图层
auto videoLayer = std::make_shared<VideoLayer>();
videoLayer->loadVideo("input.mp4");

// 2. 设置 Transform
videoLayer->transform.position = {960, 540};  // 居中
videoLayer->transform.rotation = 45.0f;       // 旋转 45 度
videoLayer->transform.scale = {1.5f, 1.5f};   // 放大 1.5 倍

// 3. 创建 Effect
auto colorAdjust = std::make_shared<ColorAdjustEffect>();

// 4. 调整 Effect 参数
colorAdjust->setParameter("brightness", 1.2f);   // 亮度 +20%
colorAdjust->setParameter("contrast", 1.1f);     // 对比度 +10%
colorAdjust->setParameter("saturation", 0.8f);   // 饱和度 -20%

// 5. 将 Effect 添加到 Layer
videoLayer->addEffect(colorAdjust);
```

---

## 🎨 Effect 参数调整

### 方式 1：使用通用的 setParameter

```cpp
auto colorAdjust = std::make_shared<ColorAdjustEffect>();

// 设置 float 参数
colorAdjust->setParameter("brightness", 1.2f);
colorAdjust->setParameter("contrast", 1.1f);
colorAdjust->setParameter("saturation", 0.9f);
colorAdjust->setParameter("exposure", 0.5f);
colorAdjust->setParameter("gain", 1.05f);
```

### 方式 2：使用类型安全的便捷方法

```cpp
auto colorAdjust = std::make_shared<ColorAdjustEffect>();

// 更安全、更清晰
colorAdjust->setBrightness(1.2f);   // 亮度
colorAdjust->setContrast(1.1f);     // 对比度
colorAdjust->setSaturation(0.9f);   // 饱和度
colorAdjust->setExposure(0.5f);     // 曝光
colorAdjust->setGain(1.05f);        // 增益
```

### 获取参数值

```cpp
float brightness = colorAdjust->getBrightness();
float contrast = colorAdjust->getContrast();

// 或使用通用方法
float saturation = colorAdjust->getParameter<float>("saturation", 1.0f);
```

### 重置参数

```cpp
// 重置单个参数到默认值
colorAdjust->resetParameter("brightness");

// 重置所有参数
colorAdjust->resetAllParameters();
```

---

## 🔗 Effect Chain（效果链）

一个 Layer 可以添加多个 Effect，按顺序应用：

```cpp
auto videoLayer = std::make_shared<VideoLayer>();

// Effect 1: 色彩调整
auto colorAdjust = std::make_shared<ColorAdjustEffect>();
colorAdjust->setBrightness(1.2f);
colorAdjust->setContrast(1.1f);
videoLayer->addEffect(colorAdjust);

// Effect 2: 模糊（待实现）
auto blur = std::make_shared<BlurEffect>();
blur->setParameter("radius", 5.0f);
videoLayer->addEffect(blur);

// Effect 3: 发光（待实现）
auto glow = std::make_shared<GlowEffect>();
glow->setParameter("intensity", 0.8f);
videoLayer->addEffect(glow);

// 应用顺序: 原始内容 → ColorAdjust → Blur → Glow → 输出
```

---

## 🛠️ Effect 管理

### 添加和移除 Effect

```cpp
// 添加 Effect
videoLayer->addEffect(colorAdjust);

// 移除 Effect（按索引）
videoLayer->removeEffect(0);  // 移除第一个

// 清空所有 Effect
videoLayer->clearEffects();
```

### 获取 Effect

```cpp
// 获取 Effect 数量
size_t count = videoLayer->getEffectCount();

// 获取所有 Effect
const auto& effects = videoLayer->getEffects();

// 获取特定 Effect
auto effect = videoLayer->getEffect(0);  // 第一个
if (effect) {
    effect->setParameter("brightness", 1.5f);
}
```

### 启用/禁用 Effect

```cpp
// 禁用 Effect（不删除，只是暂时不应用）
colorAdjust->setEnabled(false);

// 重新启用
colorAdjust->setEnabled(true);

// 检查状态
if (colorAdjust->isEnabled()) {
    // ...
}
```

---

## 📝 完整示例

### 示例 1：单个 Layer + 单个 Effect

```cpp
#include <clipengine/layers/VideoLayer.h>
#include <clipengine/effects/ColorAdjustEffect.h>

int main() {
    // 创建视频图层
    auto video = std::make_shared<VideoLayer>();
    video->initialize(device, format);
    video->loadVideo("input.mp4");

    // Transform
    video->transform.position = {960, 540};
    video->transform.scale = {1.0f, 1.0f};

    // 添加色彩调整 Effect
    auto colorAdjust = std::make_shared<ColorAdjustEffect>();
    colorAdjust->initialize(device, format);
    colorAdjust->setBrightness(1.2f);
    colorAdjust->setContrast(1.1f);
    video->addEffect(colorAdjust);

    // 渲染
    auto result = video->render(0.0f);

    return 0;
}
```

### 示例 2：多个 Layer + 多个 Effect

```cpp
#include <clipengine/layers/VideoLayer.h>
#include <clipengine/layers/ImageLayer.h>
#include <clipengine/effects/ColorAdjustEffect.h>
#include <clipengine/compositor/Compositor.h>

int main() {
    // Layer 1: 背景视频
    auto background = std::make_shared<VideoLayer>();
    background->loadVideo("background.mp4");
    background->setZOrder(0);

    // 添加 Effect
    auto bgColorAdjust = std::make_shared<ColorAdjustEffect>();
    bgColorAdjust->setBrightness(0.8f);  // 稍微变暗
    bgColorAdjust->setSaturation(0.7f);  // 降低饱和度
    background->addEffect(bgColorAdjust);

    // Layer 2: 主视频
    auto mainVideo = std::make_shared<VideoLayer>();
    mainVideo->loadVideo("main.mp4");
    mainVideo->setZOrder(1);
    mainVideo->transform.position = {960, 540};
    mainVideo->transform.scale = {0.8f, 0.8f};

    auto mainColorAdjust = std::make_shared<ColorAdjustEffect>();
    mainColorAdjust->setBrightness(1.2f);
    mainColorAdjust->setContrast(1.1f);
    mainVideo->addEffect(mainColorAdjust);

    // Layer 3: Logo 叠加
    auto logo = std::make_shared<ImageLayer>();
    logo->loadImage("logo.png");
    logo->setZOrder(2);
    logo->transform.position = {100, 100};
    logo->transform.scale = {0.3f, 0.3f};
    logo->transform.opacity = 0.8f;

    // 合成所有 Layer
    LayerStack stack;
    stack.addLayer(background);
    stack.addLayer(mainVideo);
    stack.addLayer(logo);

    Compositor compositor;
    compositor.initialize(device, format, 1920, 1080);
    auto result = compositor.render(stack, 0.0f);

    return 0;
}
```

### 示例 3：动态调整参数

```cpp
// 实时调整参数（例如在渲染循环中）
float time = 0.0f;
while (running) {
    // 动态调整亮度（呼吸效果）
    float brightness = 1.0f + 0.3f * std::sin(time * 2.0f);
    colorAdjust->setBrightness(brightness);

    // 渲染
    auto result = video->render(time);

    time += deltaTime;
}
```

### 示例 4：交互式参数调整

```cpp
// 使用 ImGui 或其他 UI 调整参数
void renderUI() {
    ImGui::Begin("Color Adjust");

    float brightness = colorAdjust->getBrightness();
    if (ImGui::SliderFloat("Brightness", &brightness, 0.0f, 2.0f)) {
        colorAdjust->setBrightness(brightness);
    }

    float contrast = colorAdjust->getContrast();
    if (ImGui::SliderFloat("Contrast", &contrast, 0.0f, 2.0f)) {
        colorAdjust->setContrast(contrast);
    }

    float saturation = colorAdjust->getSaturation();
    if (ImGui::SliderFloat("Saturation", &saturation, 0.0f, 2.0f)) {
        colorAdjust->setSaturation(saturation);
    }

    if (ImGui::Button("Reset All")) {
        colorAdjust->resetAllParameters();
    }

    ImGui::End();
}
```

---

## 🎨 ColorAdjustEffect 参数说明

### Brightness（亮度）
- **范围**: 0.0 ~ 2.0
- **默认**: 1.0
- **说明**:
  - 1.0 = 原始亮度
  - >1.0 = 变亮（如 1.2 = 亮 20%）
  - <1.0 = 变暗（如 0.8 = 暗 20%）

```cpp
colorAdjust->setBrightness(1.2f);  // 亮 20%
```

### Contrast（对比度）
- **范围**: 0.0 ~ 2.0
- **默认**: 1.0
- **说明**:
  - 1.0 = 原始对比度
  - >1.0 = 对比度增加
  - <1.0 = 对比度降低

```cpp
colorAdjust->setContrast(1.3f);  // 对比度 +30%
```

### Saturation（饱和度）
- **范围**: 0.0 ~ 2.0
- **默认**: 1.0
- **说明**:
  - 0.0 = 完全灰度
  - 1.0 = 原始饱和度
  - >1.0 = 过度饱和

```cpp
colorAdjust->setSaturation(0.0f);  // 黑白效果
colorAdjust->setSaturation(1.5f);  // 鲜艳 +50%
```

### Exposure（曝光）
- **范围**: -1.0 ~ 1.0
- **默认**: 0.0
- **说明**:
  - 0.0 = 原始曝光
  - >0.0 = 增加曝光
  - <0.0 = 降低曝光

```cpp
colorAdjust->setExposure(0.5f);   // 曝光 +0.5
colorAdjust->setExposure(-0.3f);  // 曝光 -0.3
```

### Gain（增益）
- **范围**: 0.0 ~ 2.0
- **默认**: 1.0
- **说明**:
  - 1.0 = 原始增益
  - >1.0 = 信号放大
  - <1.0 = 信号衰减

```cpp
colorAdjust->setGain(1.1f);  // 增益 +10%
```

---

## 🔄 Effect 工作流程

```
┌─────────┐
│  Layer  │
│ Content │
└────┬────┘
     │
     ▼
┌─────────────┐
│ Transform   │ ← position, rotation, scale
└─────┬───────┘
      │
      ▼
┌─────────────┐
│  Effect 1   │ ← ColorAdjust
│  Parameters │   brightness: 1.2
└─────┬───────┘   contrast: 1.1
      │           saturation: 0.9
      ▼
┌─────────────┐
│  Effect 2   │ ← Blur
│  Parameters │   radius: 5.0
└─────┬───────┘
      │
      ▼
┌─────────────┐
│   Output    │
│   Texture   │
└─────────────┘
```

---

## 💡 最佳实践

### 1. 合理的参数范围
```cpp
// ✅ 推荐：适度调整
colorAdjust->setBrightness(1.2f);   // 适度变亮
colorAdjust->setContrast(1.1f);     // 微调对比度

// ❌ 避免：过度调整
colorAdjust->setBrightness(5.0f);   // 过亮，失真
colorAdjust->setSaturation(10.0f);  // 过度饱和
```

### 2. Effect 顺序很重要
```cpp
// 顺序 1: 先色彩，后模糊
layer->addEffect(colorAdjust);  // 先调色
layer->addEffect(blur);          // 再模糊

// 顺序 2: 先模糊，后色彩
layer->addEffect(blur);          // 先模糊
layer->addEffect(colorAdjust);  // 再调色

// 结果不同！
```

### 3. 性能考虑
```cpp
// 禁用不需要的 Effect
if (!needColorAdjust) {
    colorAdjust->setEnabled(false);
}

// 减少 Effect 数量
layer->clearEffects();  // 清空后重新添加需要的
```

### 4. 参数验证
```cpp
// 确保参数在合理范围内
float brightness = std::clamp(userInput, 0.0f, 2.0f);
colorAdjust->setBrightness(brightness);
```

---

## 🔮 未来的 Effect

### 即将实现

```cpp
// Blur Effect
auto blur = std::make_shared<BlurEffect>();
blur->setParameter("radius", 5.0f);
blur->setParameter("quality", 2);  // 1=low, 2=medium, 3=high

// Glow Effect
auto glow = std::make_shared<GlowEffect>();
glow->setParameter("intensity", 0.8f);
glow->setParameter("threshold", 0.5f);

// Sharpen Effect
auto sharpen = std::make_shared<SharpenEffect>();
sharpen->setParameter("amount", 1.5f);

// Vignette Effect
auto vignette = std::make_shared<VignetteEffect>();
vignette->setParameter("intensity", 0.5f);
vignette->setParameter("radius", 0.8f);
```

---

## 📚 相关文档

- [RENDER_CORE_SIMPLE.md](./RENDER_CORE_SIMPLE.md) - 架构设计
- [SIMPLE_API_GUIDE.md](./SIMPLE_API_GUIDE.md) - 完整 API 指南
- [IMPLEMENTATION_ROADMAP.md](./IMPLEMENTATION_ROADMAP.md) - 实现路线图

---

**最后更新**: 2025-11-06
**架构**: Layer + Effect Chain + Compositor
**核心理念**: Layer 可以添加 Effect，Effect 可以调整参数 ✨
