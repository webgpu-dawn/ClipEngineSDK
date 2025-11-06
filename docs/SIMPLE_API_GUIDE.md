# ClipEngine Simple API Guide

> 🎯 **简化的图层合成 API** - 基于 Layer + Transform + Filter Chain + Compositor 架构

---

## 📚 API 概述

ClipEngine 提供了一个清晰、友好的 API，灵感来自 After Effects/Premiere Pro 的概念：

```
Layer (图层) → Transform (变换) → Filter Chain (滤镜链) → Compositor (合成器) → Output (输出)
```

---

## 🏗️ 核心概念

### 1. Layer (图层)

图层是可渲染的视觉元素，包括：

- **VideoLayer** - 视频图层
- **ImageLayer** - 图片图层
- **SolidLayer** - 纯色图层
- **TextLayer** - 文字图层（待实现）

### 2. Transform2D (变换)

每个图层都有一个 `transform` 属性：

```cpp
layer->transform.position = {100.0f, 200.0f};  // 位置
layer->transform.rotation = 45.0f;              // 旋转（度）
layer->transform.scale = {1.5f, 1.5f};          // 缩放
layer->transform.anchor = {0.5f, 0.5f};         // 锚点（0.5 = 中心）
layer->transform.opacity = 0.8f;                // 不透明度
```

### 3. Filter Chain (滤镜链)

每个图层可以添加多个滤镜：

```cpp
layer->addFilter(std::make_shared<GaussianBlur>(5.0f));
layer->addFilter(std::make_shared<ColorAdjust>(brightness, contrast));
```

### 4. Compositor (合成器)

合成器按 Z-Order 合成所有图层：

```cpp
background->setZOrder(0);  // 底层
video->setZOrder(1);        // 中间层
overlay->setZOrder(2);      // 顶层
```

---

## 📋 快速开始

### 示例 1：简单视频渲染

```cpp
#include <clipengine/SimpleAPI.h>

using namespace clipengine;

int main() {
    // 1. 初始化引擎
    SimpleClipEngine engine;
    engine.initialize(1920, 1080);

    // 2. 创建视频图层
    auto video = engine.createVideoLayer("input.mp4", "Main Video");

    // 3. 渲染
    engine.render();

    return 0;
}
```

### 示例 2：多图层合成

```cpp
#include <clipengine/SimpleAPI.h>

using namespace clipengine;

int main() {
    SimpleClipEngine engine;
    engine.initialize(1920, 1080);

    // 创建背景层
    auto background = engine.createSolidLayer({0.1f, 0.1f, 0.1f, 1.0f}, "Background");
    background->setZOrder(0);

    // 创建主视频层
    auto video = engine.createVideoLayer("video.mp4", "Main Video");
    video->setZOrder(1);
    video->transform.position = {960.0f, 540.0f};  // 居中

    // 创建 Logo 叠加层
    auto logo = engine.createImageLayer("logo.png", "Logo");
    logo->setZOrder(2);
    logo->transform.position = {100.0f, 100.0f};   // 左上角
    logo->transform.scale = {0.3f, 0.3f};          // 缩小到 30%
    logo->transform.opacity = 0.8f;                 // 80% 不透明度

    // 渲染
    float time = 0.0f;
    while (running) {
        engine.update(deltaTime);
        engine.render(time);
        engine.present();
        time += deltaTime;
    }

    return 0;
}
```

### 示例 3：添加滤镜效果

```cpp
#include <clipengine/SimpleAPI.h>
#include <clipengine/filters/ColorAdjust.h>
#include <clipengine/filters/GaussianBlur.h>

using namespace clipengine;

int main() {
    SimpleClipEngine engine;
    engine.initialize(1920, 1080);

    // 创建视频层
    auto video = engine.createVideoLayer("input.mp4");

    // 添加色彩调整滤镜
    auto colorAdjust = std::make_shared<ColorAdjustFilter>();
    colorAdjust->setBrightness(1.2f);
    colorAdjust->setContrast(1.1f);
    colorAdjust->setSaturation(1.3f);
    video->addFilter(colorAdjust);

    // 添加模糊滤镜
    auto blur = std::make_shared<GaussianBlur>(3.0f);
    video->addFilter(blur);

    // 渲染
    engine.render();

    return 0;
}
```

### 示例 4：360° 全景视频

```cpp
#include <clipengine/SimpleAPI.h>

using namespace clipengine;

int main() {
    SimpleClipEngine engine;
    engine.initialize(1920, 1080);

    // 创建全景视频层
    auto panorama = engine.createVideoLayer("360_video.mp4");

    // 设置为全景投影模式
    panorama->setProjection(VideoProjection::Equirectangular);

    // 设置初始视角
    panorama->setRotation(0.0f, 0.0f);  // yaw, pitch
    panorama->setZoom(1.0f);

    // 交互式旋转（示例）
    float yaw = 0.0f;
    while (running) {
        // 鼠标拖拽改变视角
        if (mouseDragging) {
            yaw += mouseDeltaX * 0.01f;
            panorama->setRotation(yaw, pitch);
        }

        engine.render();
        engine.present();
    }

    return 0;
}
```

### 示例 5：画中画（PiP）效果

```cpp
#include <clipengine/SimpleAPI.h>

using namespace clipengine;

int main() {
    SimpleClipEngine engine;
    engine.initialize(1920, 1080);

    // 主视频（全屏）
    auto mainVideo = engine.createVideoLayer("main.mp4", "Main");
    mainVideo->setZOrder(0);

    // PiP 小窗口视频
    auto pipVideo = engine.createVideoLayer("pip.mp4", "PiP");
    pipVideo->setZOrder(1);
    pipVideo->transform.position = {1920.0f - 320.0f - 20.0f, 20.0f};  // 右上角
    pipVideo->transform.scale = {0.25f, 0.25f};  // 缩小到 25%

    // 添加边框（使用叠加层）
    auto border = engine.createSolidLayer({1.0f, 1.0f, 1.0f, 0.3f}, "Border");
    border->setZOrder(2);
    border->transform.position = pipVideo->transform.position;
    border->transform.scale = {0.26f, 0.26f};  // 稍大一点

    engine.render();

    return 0;
}
```

---

## 🎨 Layer API 详解

### VideoLayer

```cpp
// 创建和加载
auto video = std::make_shared<VideoLayer>();
video->initialize(device, format);
video->loadVideo("path/to/video.mp4");

// 投影模式
video->setProjection(VideoProjection::Planar);         // 平面
video->setProjection(VideoProjection::Equirectangular); // 360°全景
video->setProjection(VideoProjection::LittlePlanet);   // 小行星
video->setProjection(VideoProjection::CrystalBall);    // 水晶球

// 全景视频控制
video->setRotation(yaw, pitch);  // 旋转（弧度）
video->setZoom(1.5f);             // 缩放

// 视频属性
uint32_t width = video->getWidth();
uint32_t height = video->getHeight();
glm::vec2 size = video->getSize();
```

### ImageLayer

```cpp
// 创建和加载
auto image = std::make_shared<ImageLayer>();
image->initialize(device, format);
image->loadImage("path/to/image.png");

// 从内存加载
image->loadImage(pixelData, width, height);

// 图片属性
uint32_t width = image->getWidth();
uint32_t height = image->getHeight();
```

### SolidLayer

```cpp
// 创建纯色层
auto solid = std::make_shared<SolidLayer>();
solid->initialize(device, format);

// 设置颜色
solid->setColor({1.0f, 0.0f, 0.0f, 1.0f});  // 红色 RGBA
solid->setColor(1.0f, 0.0f, 0.0f, 1.0f);    // 红色 (r, g, b, a)

// 设置大小
solid->setSize(1920, 1080);
```

---

## 🔄 Transform API 详解

```cpp
// 位置（像素坐标）
layer->transform.position = {100.0f, 200.0f};

// 旋转（度）
layer->transform.rotation = 45.0f;  // 顺时针旋转 45°

// 缩放
layer->transform.scale = {1.5f, 1.5f};  // 150% 大小
layer->transform.scale = {2.0f, 1.0f};  // 水平拉伸 2 倍

// 锚点（归一化坐标 0.0-1.0）
layer->transform.anchor = {0.5f, 0.5f};  // 中心（默认）
layer->transform.anchor = {0.0f, 0.0f};  // 左上角
layer->transform.anchor = {1.0f, 1.0f};  // 右下角

// 不透明度
layer->transform.opacity = 0.8f;  // 80% 不透明度

// 获取变换矩阵
glm::mat4 matrix = layer->transform.toMatrix(layerSize);
```

---

## 🎨 Filter API 详解

### 色彩调整

```cpp
auto colorAdjust = std::make_shared<ColorAdjustFilter>();
colorAdjust->setBrightness(1.2f);   // 亮度 (1.0 = 原始)
colorAdjust->setContrast(1.1f);     // 对比度
colorAdjust->setSaturation(1.3f);   // 饱和度
colorAdjust->setExposure(0.5f);     // 曝光
layer->addFilter(colorAdjust);
```

### 模糊

```cpp
auto blur = std::make_shared<GaussianBlur>(5.0f);  // 半径 5 像素
layer->addFilter(blur);
```

### 自定义滤镜

```cpp
class MyCustomFilter : public Filter {
public:
    bool initialize(wgpu::Device device, wgpu::TextureFormat format) override {
        // 初始化着色器、管线等
    }

    wgpu::TextureView apply(wgpu::TextureView input, float time) override {
        // 应用滤镜效果
    }
};

auto customFilter = std::make_shared<MyCustomFilter>();
layer->addFilter(customFilter);
```

---

## 🏗️ Compositor API 详解

```cpp
Compositor compositor;
compositor.initialize(device, format, 1920, 1080);

// 设置背景色
compositor.setBackgroundColor({0.0f, 0.0f, 0.0f, 1.0f});

// 渲染图层栈
LayerStack stack;
stack.addLayer(layer1);
stack.addLayer(layer2);
wgpu::TextureView result = compositor.render(stack, currentTime);

// 更改分辨率
compositor.setResolution(3840, 2160);  // 4K
```

---

## 📊 完整工作流程

```cpp
#include <clipengine/SimpleAPI.h>

using namespace clipengine;

int main() {
    // 1. 初始化引擎
    SimpleClipEngine engine;
    if (!engine.initialize(1920, 1080)) {
        return -1;
    }

    // 2. 创建图层
    auto background = engine.createSolidLayer({0.2f, 0.2f, 0.2f, 1.0f}, "BG");
    auto video1 = engine.createVideoLayer("video1.mp4", "Video 1");
    auto video2 = engine.createVideoLayer("video2.mp4", "Video 2");
    auto logo = engine.createImageLayer("logo.png", "Logo");

    // 3. 设置 Z-Order
    background->setZOrder(0);
    video1->setZOrder(1);
    video2->setZOrder(2);
    logo->setZOrder(3);

    // 4. 设置变换
    video1->transform.position = {0.0f, 0.0f};
    video2->transform.position = {1920.0f - 480.0f, 1080.0f - 270.0f};
    video2->transform.scale = {0.25f, 0.25f};
    logo->transform.position = {50.0f, 50.0f};

    // 5. 添加滤镜
    auto colorAdjust = std::make_shared<ColorAdjustFilter>();
    colorAdjust->setBrightness(1.2f);
    video1->addFilter(colorAdjust);

    // 6. 渲染循环
    float time = 0.0f;
    float deltaTime = 1.0f / 60.0f;

    while (running) {
        engine.update(deltaTime);
        engine.render(time);
        engine.present();

        time += deltaTime;
    }

    // 7. 清理
    engine.shutdown();

    return 0;
}
```

---

## ⚡ 性能优化建议

### 1. 图层缓存

```cpp
// 如果图层内容不变，引擎会自动缓存
layer->setVisible(false);  // 暂时隐藏，不会重新渲染
```

### 2. 滤镜优化

```cpp
// 只在需要时启用滤镜
filter->setEnabled(false);  // 禁用滤镜

// 调整滤镜顺序（耗时的放后面）
layer->addFilter(cheapFilter);
layer->addFilter(expensiveFilter);
```

### 3. 分辨率控制

```cpp
// 使用适当的分辨率
engine.setResolution(1920, 1080);  // Full HD
engine.setResolution(1280, 720);   // HD (更快)
```

---

## 🎯 API 设计原则

1. **简单优先** - 常用操作应该简单直观
2. **强大灵活** - 高级功能仍然可访问
3. **类型安全** - 使用强类型，避免错误
4. **符合直觉** - API 命名清晰易懂
5. **性能优化** - 自动缓存和优化

---

## 🔗 相关文档

- [RENDER_CORE_SIMPLE.md](./RENDER_CORE_SIMPLE.md) - 架构设计
- [API_REFERENCE.md](./API_REFERENCE.md) - 完整 API 参考
- [EXAMPLES.md](./EXAMPLES.md) - 更多示例代码

---

**最后更新**: 2025-11-06
**ClipEngineSDK Version**: v2.0
**架构**: Layer + Transform + Filter Chain + Compositor
