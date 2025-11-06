# ClipEngine Implementation Roadmap

> 🗺️ **架构重构实施路线图** - 从设计到实现

---

## ✅ 已完成：架构设计

### 1. 核心头文件

以下头文件已创建，定义了新架构的接口：

#### Transform System
- ✅ `src/clipengine/transform/Transform2D.h`
  - Position, Rotation, Scale, Anchor, Opacity
  - Matrix generation
  - Dirty flag tracking

#### Layer System
- ✅ `src/clipengine/layers/Layer.h` - Layer 基类
  - 统一的图层接口
  - Transform, Filter Chain, Time Range
  - Visibility, Opacity, Blend Mode, Z-Order

- ✅ `src/clipengine/layers/VideoLayer.h`
  - 视频图层实现
  - 多格式支持（NV12, I420, RGBA）
  - 多投影模式（Planar, 360°, Little Planet, Crystal Ball）

- ✅ `src/clipengine/layers/ImageLayer.h`
  - 图片图层实现
  - 支持 PNG, JPEG, BMP 等格式

- ✅ `src/clipengine/layers/SolidLayer.h`
  - 纯色图层实现
  - RGBA 颜色支持

#### Compositor System
- ✅ `src/clipengine/compositor/Compositor.h`
  - LayerStack - 图层栈管理
  - Compositor - 多图层合成
  - Z-Order 排序
  - Blend Mode 支持

#### Simple API
- ✅ `src/clipengine/SimpleAPI.h`
  - SimpleClipEngine - 简化的高层 API
  - 便捷的图层创建方法
  - 自动化的资源管理

### 2. 文档

- ✅ `docs/SIMPLE_API_GUIDE.md`
  - 完整的 API 使用指南
  - 多个示例代码
  - 最佳实践

- ✅ `docs/ARCHITECTURE_REFACTOR.md`
  - 架构对比
  - 迁移指南
  - 最佳实践

- ✅ `docs/IMPLEMENTATION_ROADMAP.md`（本文档）
  - 实施计划
  - 任务清单

---

## 🚧 待实现：CPP 实现

### Phase 1: 核心基础 (1-2 周)

#### 1.1 Transform2D 实现
```
📁 src/clipengine/transform/Transform2D.cpp
```

**任务**：
- [ ] 实现 `toMatrix()` 方法
- [ ] 测试矩阵生成正确性
- [ ] 添加 dirty flag 管理

**依赖**：GLM 库

#### 1.2 Layer 基类实现
```
📁 src/clipengine/layers/Layer.cpp
```

**任务**：
- [ ] 实现 `applyFilters()` 方法
- [ ] 实现基础的 GPU 资源管理
- [ ] 添加时间范围检查

**依赖**：Filter 系统

#### 1.3 SolidLayer 实现
```
📁 src/clipengine/layers/SolidLayer.cpp
```

**任务**：
- [ ] 实现 `initialize()` - 创建 GPU 纹理
- [ ] 实现 `render()` - 渲染纯色
- [ ] 实现 `updateTexture()` - 更新颜色
- [ ] 创建 WGSL 着色器

**优先级**：⭐⭐⭐（最简单，先实现）

---

### Phase 2: 图层实现 (2-3 周)

#### 2.1 ImageLayer 实现
```
📁 src/clipengine/layers/ImageLayer.cpp
```

**任务**：
- [ ] 实现 `loadImage()` - 从文件加载
- [ ] 集成 stb_image 解码
- [ ] 实现 `render()` - 渲染图片
- [ ] 创建 WGSL 着色器

**依赖**：stb_image 库

**优先级**：⭐⭐⭐

#### 2.2 VideoLayer 实现
```
📁 src/clipengine/layers/VideoLayer.cpp
```

**任务**：
- [ ] 从 VideoRenderer 迁移核心代码
- [ ] 实现 `loadVideo()` - 从文件加载
- [ ] 实现 `updateFrame()` - D3D11 interop
- [ ] 实现多投影模式着色器
- [ ] 实现全景视频控制

**依赖**：FFmpeg, D3D11

**优先级**：⭐⭐⭐

---

### Phase 3: Compositor 实现 (1-2 周)

#### 3.1 LayerStack 实现
```
📁 src/clipengine/compositor/Compositor.cpp (LayerStack 部分)
```

**任务**：
- [ ] 实现图层添加/删除
- [ ] 实现 Z-Order 排序
- [ ] 实现按名称查找

**优先级**：⭐⭐⭐

#### 3.2 Compositor 实现
```
📁 src/clipengine/compositor/Compositor.cpp
```

**任务**：
- [ ] 实现 `initialize()` - 创建渲染目标
- [ ] 实现 `render()` - 渲染所有图层
- [ ] 实现 `compositeLayer()` - 单个图层合成
- [ ] 实现 Blend Mode 着色器
- [ ] 实现 Transform 应用

**依赖**：Layer, Transform2D

**优先级**：⭐⭐⭐

---

### Phase 4: Simple API 实现 (1 周)

#### 4.1 SimpleClipEngine 实现
```
📁 src/clipengine/SimpleAPI.cpp
```

**任务**：
- [ ] 实现 `initialize()` - 初始化引擎
- [ ] 实现图层创建方法
- [ ] 实现图层管理方法
- [ ] 实现渲染循环
- [ ] 资源自动管理

**依赖**：Compositor, Layer, RenderDevice

**优先级**：⭐⭐

---

### Phase 5: 测试和示例 (1-2 周)

#### 5.1 单元测试
```
📁 tests/layers/
📁 tests/compositor/
📁 tests/transform/
```

**任务**：
- [ ] Transform2D 测试
- [ ] SolidLayer 测试
- [ ] ImageLayer 测试
- [ ] VideoLayer 测试
- [ ] Compositor 测试

#### 5.2 示例程序
```
📁 samples/simple_api_example/
```

**任务**：
- [ ] 创建 simple_video_example
- [ ] 创建 multi_layer_example
- [ ] 创建 panorama_example
- [ ] 创建 pip_example

**优先级**：⭐⭐

---

## 📋 实施顺序建议

### Week 1-2: 基础实现
1. ✅ Transform2D.cpp
2. ✅ Layer.cpp
3. ✅ SolidLayer.cpp (最简单)
4. ✅ 测试 SolidLayer

### Week 3-4: 图层实现
5. ✅ ImageLayer.cpp
6. ✅ 测试 ImageLayer
7. ✅ VideoLayer.cpp (从 VideoRenderer 迁移)
8. ✅ 测试 VideoLayer

### Week 5-6: Compositor
9. ✅ LayerStack 实现
10. ✅ Compositor.cpp
11. ✅ Blend Mode 着色器
12. ✅ 测试 Compositor

### Week 7: Simple API
13. ✅ SimpleClipEngine.cpp
14. ✅ 资源管理
15. ✅ 测试 Simple API

### Week 8: 示例和文档
16. ✅ 创建示例程序
17. ✅ 更新文档
18. ✅ 性能优化

---

## 🔧 技术细节

### 1. Transform2D 实现

```cpp
// src/clipengine/transform/Transform2D.cpp

#include "Transform2D.h"

namespace clipengine {

glm::mat4 Transform2D::toMatrix(const glm::vec2& layerSize) const {
    glm::mat4 mat = glm::mat4(1.0f);

    // 计算锚点偏移（像素坐标）
    glm::vec2 anchorOffset = anchor * layerSize;

    // 1. 平移到锚点（使锚点成为原点）
    mat = glm::translate(mat, glm::vec3(-anchorOffset, 0.0f));

    // 2. 缩放
    mat = glm::scale(mat, glm::vec3(scale, 1.0f));

    // 3. 旋转（绕锚点）
    mat = glm::rotate(mat, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));

    // 4. 平移到目标位置
    mat = glm::translate(mat, glm::vec3(position + anchorOffset, 0.0f));

    return mat;
}

} // namespace clipengine
```

### 2. SolidLayer 实现

```cpp
// src/clipengine/layers/SolidLayer.cpp

#include "SolidLayer.h"

namespace clipengine {

bool SolidLayer::initialize(wgpu::Device device, wgpu::TextureFormat format) {
    device_ = device;
    surfaceFormat_ = format;

    // 创建纯色纹理
    createTexture();

    // 创建渲染管线
    createPipeline();

    return true;
}

wgpu::TextureView SolidLayer::render(float time) {
    if (!visible_) {
        return nullptr;
    }

    // 如果颜色改变，更新纹理
    if (colorDirty_) {
        updateTexture();
        colorDirty_ = false;
    }

    return textureView_;
}

void SolidLayer::createTexture() {
    wgpu::TextureDescriptor desc = {};
    desc.size = {width_, height_, 1};
    desc.format = wgpu::TextureFormat::RGBA8Unorm;
    desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;

    texture_ = device_.CreateTexture(&desc);
    textureView_ = texture_.CreateView();
}

void SolidLayer::updateTexture() {
    // 使用 render pass 清除为指定颜色
    wgpu::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = textureView_;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = {color_.r, color_.g, color_.b, color_.a};

    wgpu::RenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;

    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    device_.GetQueue().Submit(1, &commands);
}

} // namespace clipengine
```

### 3. Compositor 实现

```cpp
// src/clipengine/compositor/Compositor.cpp

#include "Compositor.h"

namespace clipengine {

wgpu::TextureView Compositor::render(const LayerStack& stack, float time) {
    // 创建 render pass
    wgpu::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = compositeTextureView_;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = {
        backgroundColor_.r,
        backgroundColor_.g,
        backgroundColor_.b,
        backgroundColor_.a
    };

    wgpu::RenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;

    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    // 渲染每个图层（按 Z-Order 排序）
    for (const auto& layer : stack.getLayers()) {
        if (!layer->isVisible()) continue;
        if (!layer->isActiveAtTime(time)) continue;

        compositeLayer(pass, layer, time);
    }

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    device_.GetQueue().Submit(1, &commands);

    return compositeTextureView_;
}

void Compositor::compositeLayer(
    wgpu::RenderPassEncoder& pass,
    const std::shared_ptr<Layer>& layer,
    float time
) {
    // 1. 渲染图层到临时纹理
    wgpu::TextureView layerTexture = layer->render(time);
    if (!layerTexture) return;

    // 2. 应用 Transform
    applyLayerTransform(pass, layer);

    // 3. 应用 Blend Mode
    applyBlendMode(pass, layer->getBlendMode());

    // 4. 合成到输出
    pass.SetPipeline(compositePipeline_);
    pass.SetBindGroup(0, /* bind group with layerTexture */);
    pass.Draw(6, 1, 0, 0);  // Full-screen quad
}

} // namespace clipengine
```

---

## 📦 CMake 配置更新

### src/clipengine/CMakeLists.txt

```cmake
# 新增源文件
set(CLIPENGINE_SOURCES
    # ... 现有文件 ...

    # Transform
    transform/Transform2D.cpp

    # Layers
    layers/Layer.cpp
    layers/VideoLayer.cpp
    layers/ImageLayer.cpp
    layers/SolidLayer.cpp

    # Compositor
    compositor/Compositor.cpp

    # Simple API
    SimpleAPI.cpp
)

# 新增头文件
set(CLIPENGINE_HEADERS
    # ... 现有文件 ...

    transform/Transform2D.h
    layers/Layer.h
    layers/VideoLayer.h
    layers/ImageLayer.h
    layers/SolidLayer.h
    compositor/Compositor.h
    SimpleAPI.h
)
```

---

## 🧪 测试计划

### 1. Transform2D 测试
```cpp
TEST(Transform2D, MatrixGeneration) {
    Transform2D transform;
    transform.position = {100.0f, 200.0f};
    transform.rotation = 45.0f;
    transform.scale = {2.0f, 2.0f};

    glm::mat4 matrix = transform.toMatrix({100.0f, 100.0f});

    // 验证矩阵正确性
    // ...
}
```

### 2. SolidLayer 测试
```cpp
TEST(SolidLayer, ColorRendering) {
    SolidLayer layer({1.0f, 0.0f, 0.0f, 1.0f});
    layer.initialize(device, format);

    wgpu::TextureView result = layer.render(0.0f);

    // 验证输出为红色
    // ...
}
```

### 3. Compositor 测试
```cpp
TEST(Compositor, MultiLayerComposite) {
    LayerStack stack;
    stack.addLayer(backgroundLayer);
    stack.addLayer(videoLayer);
    stack.addLayer(overlayLayer);

    Compositor compositor;
    compositor.initialize(device, format, 1920, 1080);

    wgpu::TextureView result = compositor.render(stack, 0.0f);

    // 验证合成结果
    // ...
}
```

---

## 🎯 成功标准

### Phase 1 完成标准
- ✅ Transform2D 可以生成正确的变换矩阵
- ✅ SolidLayer 可以渲染纯色
- ✅ 单元测试通过

### Phase 2 完成标准
- ✅ ImageLayer 可以加载和渲染图片
- ✅ VideoLayer 可以播放视频
- ✅ 所有投影模式正常工作

### Phase 3 完成标准
- ✅ Compositor 可以合成多个图层
- ✅ Z-Order 排序正确
- ✅ Blend Mode 正常工作

### Phase 4 完成标准
- ✅ SimpleClipEngine API 可用
- ✅ 示例程序运行正常
- ✅ 文档完整

---

## 📚 相关文档

- [RENDER_CORE_SIMPLE.md](./RENDER_CORE_SIMPLE.md) - 架构设计
- [SIMPLE_API_GUIDE.md](./SIMPLE_API_GUIDE.md) - API 使用指南
- [ARCHITECTURE_REFACTOR.md](./ARCHITECTURE_REFACTOR.md) - 架构重构说明

---

**创建日期**: 2025-11-06
**预计完成**: 2025-12-01 (4 周)
**当前状态**: Phase 0 - 设计完成，开始实现
