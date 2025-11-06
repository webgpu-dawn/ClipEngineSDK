# ClipEngine Architecture Refactor

> 📦 **从复杂到简单** - 基于 Layer + Compositor 概念的架构重构

---

## 🎯 重构目标

提高 SDK 的易用性和友好程度，让用户能够：

1. **快速上手** - 简单任务简单实现
2. **清晰理解** - 架构概念清晰明了
3. **灵活扩展** - 复杂需求也能满足

---

## 📊 架构对比

### 旧架构

```
CompositionEngine
├── VideoRenderer
├── TextureRenderer
├── ShaderEffect
└── FilterChain
```

**问题**：
- 概念不清晰（Renderer vs Layer）
- API 复杂（需要理解底层细节）
- 扩展困难（缺少统一的图层抽象）

### 新架构

```
SimpleClipEngine
├── LayerStack
│   ├── VideoLayer
│   ├── ImageLayer
│   ├── SolidLayer
│   └── TextLayer
├── Transform2D
├── FilterChain
└── Compositor
```

**优点**：
- ✅ 概念清晰（Layer + Transform + Filter + Compositor）
- ✅ API 简单（高层封装，易于使用）
- ✅ 易于扩展（统一的 Layer 基类）

---

## 🏗️ 核心组件

### 1. Layer System (图层系统)

#### 旧架构
```cpp
// 创建视频渲染器
VideoRenderer* videoRenderer = engine.createLayer<VideoRenderer>();
videoRenderer->setRenderMode(VideoRenderer::RenderMode::Equirectangular);
```

#### 新架构
```cpp
// 创建视频图层
auto video = std::make_shared<VideoLayer>();
video->setProjection(VideoProjection::Equirectangular);
video->transform.position = {960, 540};
video->transform.rotation = 45.0f;
```

**改进**：
- 使用 `Layer` 而非 `Renderer` 概念（更贴近 AE/Premiere）
- 内置 `transform` 属性（无需额外设置）
- 类型化的图层（VideoLayer, ImageLayer, SolidLayer）

### 2. Transform System (变换系统)

#### 旧架构
```cpp
// 使用 LayerTransform 结构
LayerTransform transform;
transform.x = 0.5f;  // 归一化坐标
transform.y = 0.5f;
transform.width = 0.5f;
transform.height = 0.5f;
layer->setTransform(transform.x, transform.y, transform.width, transform.height);
```

#### 新架构
```cpp
// 使用 Transform2D 类
layer->transform.position = {100.0f, 200.0f};  // 像素坐标
layer->transform.rotation = 45.0f;              // 度
layer->transform.scale = {1.5f, 1.5f};          // 缩放
layer->transform.anchor = {0.5f, 0.5f};         // 锚点
layer->transform.opacity = 0.8f;                // 不透明度
```

**改进**：
- 完整的 2D 变换支持（Position + Rotation + Scale + Anchor）
- 像素坐标（更直观）
- 矩阵计算自动化

### 3. Filter Chain (滤镜链)

#### 旧架构
```cpp
ShaderEffect* colorAdjust = engine.createEffect("color_adjust");
colorAdjust->setParameter("brightness", 0.2f);
videoRenderer->addEffect(colorAdjust);
```

#### 新架构
```cpp
auto colorAdjust = std::make_shared<ColorAdjustFilter>();
colorAdjust->setBrightness(1.2f);
colorAdjust->setContrast(1.1f);
layer->addFilter(colorAdjust);
```

**改进**：
- 类型安全的 Filter 类（不是字符串）
- 强类型的参数设置（setBrightness vs setParameter）
- 每个 Layer 独立的滤镜链

### 4. Compositor (合成器)

#### 旧架构
```cpp
// 在 CompositionEngine 内部自动合成
engine.render();
```

#### 新架构
```cpp
// 显式的 Compositor 概念
LayerStack stack;
stack.addLayer(background);
stack.addLayer(video);
stack.addLayer(overlay);

Compositor compositor;
compositor.render(stack, time);
```

**改进**：
- 显式的 LayerStack 概念
- Z-Order 管理清晰
- 合成过程可控

---

## 📂 新目录结构

```
src/clipengine/
├── compositor/          # 新增：合成器模块
│   └── Compositor.h/cpp
│
├── layers/              # 增强：清晰的图层类型
│   ├── Layer.h/cpp         # 新增：Layer 基类
│   ├── VideoLayer.h/cpp    # 重构：VideoRenderer → VideoLayer
│   ├── ImageLayer.h/cpp    # 新增
│   ├── SolidLayer.h/cpp    # 新增
│   └── TextLayer.h/cpp     # 待实现
│
├── transform/           # 新增：变换系统
│   ├── Transform2D.h
│   └── Transform3D.h       # 可选
│
├── filters/             # 重命名：effects → filters
│   ├── Filter.h/cpp
│   ├── FilterChain.h/cpp
│   └── ... 各种具体滤镜
│
├── SimpleAPI.h          # 新增：简化 API
│
└── core/                # 保留：核心渲染
    ├── RenderDevice.h/cpp
    └── ...
```

---

## 🔄 迁移指南

### 从旧 API 迁移到新 API

#### 1. 初始化引擎

**旧代码**：
```cpp
CompositionEngine engine;
DeviceConfig config = {
    .width = 1920,
    .height = 1080,
    .hwnd = windowHandle
};
engine.initialize(config);
```

**新代码**：
```cpp
SimpleClipEngine engine;
engine.initialize(1920, 1080, windowHandle);
```

#### 2. 创建视频图层

**旧代码**：
```cpp
VideoRenderer* videoRenderer = engine.createLayer<VideoRenderer>();
videoRenderer->setLayer(0);
videoRenderer->setName("Main Video");
videoRenderer->setRenderMode(VideoRenderer::RenderMode::Panorama);
```

**新代码**：
```cpp
auto video = engine.createVideoLayer("video.mp4", "Main Video");
video->setZOrder(0);
video->setProjection(VideoProjection::Equirectangular);
```

#### 3. 设置变换

**旧代码**：
```cpp
videoRenderer->setTransform(0.0f, 0.0f, 1.0f, 1.0f);  // 归一化坐标
```

**新代码**：
```cpp
video->transform.position = {960.0f, 540.0f};  // 像素坐标
video->transform.scale = {1.0f, 1.0f};
video->transform.rotation = 0.0f;
```

#### 4. 添加滤镜

**旧代码**：
```cpp
ShaderEffect* colorAdjust = engine.createEffect("color_adjust");
colorAdjust->setParameter("brightness", 0.2f);
videoRenderer->addEffect(colorAdjust);
```

**新代码**：
```cpp
auto colorAdjust = std::make_shared<ColorAdjustFilter>();
colorAdjust->setBrightness(1.2f);
video->addFilter(colorAdjust);
```

#### 5. 渲染

**旧代码**：
```cpp
engine.render();
```

**新代码**：
```cpp
engine.render(currentTime);
```

---

## 🎨 新 API 特性

### 1. 链式调用支持

```cpp
auto video = engine.createVideoLayer("video.mp4")
    ->setProjection(VideoProjection::Equirectangular)
    ->setRotation(yaw, pitch)
    ->setZoom(1.5f);

video->transform
    .position = {960, 540};
    .rotation = 45.0f;
    .scale = {1.5f, 1.5f};
```

### 2. 图层管理

```cpp
// 添加图层
engine.addLayer(videoLayer);

// 移除图层
engine.removeLayer(videoLayer);
engine.removeLayerByName("Main Video");

// 获取图层
auto layer = engine.getLayerByName("Main Video");

// 清空所有图层
engine.clearLayers();
```

### 3. Z-Order 管理

```cpp
background->setZOrder(0);   // 底层
video->setZOrder(1);         // 中层
overlay->setZOrder(2);       // 顶层

// LayerStack 自动按 Z-Order 排序
```

### 4. 滤镜链管理

```cpp
// 添加滤镜
layer->addFilter(blur);
layer->addFilter(colorAdjust);

// 移除滤镜
layer->removeFilter(0);

// 清空滤镜
layer->clearFilters();

// 获取所有滤镜
const auto& filters = layer->getFilters();
```

---

## 💡 最佳实践

### 1. 使用智能指针

```cpp
// 推荐
auto layer = std::make_shared<VideoLayer>();
auto filter = std::make_shared<GaussianBlur>(5.0f);

// 不推荐
VideoLayer* layer = new VideoLayer();  // 需要手动删除
```

### 2. 合理设置 Z-Order

```cpp
// 背景层
background->setZOrder(0);

// 主内容层
for (int i = 0; i < videos.size(); i++) {
    videos[i]->setZOrder(10 + i);
}

// UI 叠加层
overlay->setZOrder(100);
```

### 3. 图层命名

```cpp
// 使用有意义的名称
auto background = engine.createSolidLayer(color, "Background");
auto mainVideo = engine.createVideoLayer(path, "Main Video");
auto logo = engine.createImageLayer(path, "Company Logo");

// 方便后续查找
auto layer = engine.getLayerByName("Main Video");
```

### 4. Transform 设置

```cpp
// 居中显示
layer->transform.position = {width / 2.0f, height / 2.0f};
layer->transform.anchor = {0.5f, 0.5f};  // 中心锚点

// 左上角
layer->transform.position = {0.0f, 0.0f};
layer->transform.anchor = {0.0f, 0.0f};  // 左上角锚点
```

---

## 📈 性能对比

### 旧架构
- 渲染开销：中等
- 内存占用：中等
- 扩展性：一般

### 新架构
- 渲染开销：**优化**（图层缓存）
- 内存占用：**优化**（智能指针管理）
- 扩展性：**优秀**（统一 Layer 接口）

---

## 🔮 未来计划

### 短期（1-2 月）
- [ ] 实现 TextLayer (文字图层)
- [ ] 实现更多 Filter (模糊、扭曲、风格化)
- [ ] 实现 Layer 动画系统（关键帧）

### 中期（3-6 月）
- [ ] 3D Transform 支持
- [ ] 粒子系统
- [ ] 音频图层和同步

### 长期（6-12 月）
- [ ] 时间轴编辑器
- [ ] GPU 加速滤镜
- [ ] 实时预览优化

---

## 🎯 总结

### 核心改进

1. **架构更清晰**
   - Layer + Transform + Filter + Compositor
   - 概念对应 AE/Premiere，易于理解

2. **API 更友好**
   - 高层封装，简单易用
   - 类型安全，减少错误

3. **扩展更容易**
   - 统一的 Layer 基类
   - 模块化设计

### 设计原则

✅ **简单优先** - 常用操作应该简单直观
✅ **强大灵活** - 高级功能仍然可访问
✅ **类型安全** - 使用强类型，避免错误
✅ **性能优化** - 自动缓存和优化

---

## 📚 相关文档

- [RENDER_CORE_SIMPLE.md](./RENDER_CORE_SIMPLE.md) - 架构设计
- [SIMPLE_API_GUIDE.md](./SIMPLE_API_GUIDE.md) - API 使用指南
- [API_REFERENCE.md](./API_REFERENCE.md) - 完整 API 参考

---

**最后更新**: 2025-11-06
**ClipEngineSDK Version**: v2.0
**架构**: Layer + Transform + Filter Chain + Compositor
