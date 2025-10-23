# ClipEngine 渲染架构说明

## 核心理念

**滤镜和特效的本质 = Shader + Uniform 参数**

这是我们架构的核心思想。所有的视觉效果都可以归结为：
1. **Shader 代码**：定义像素处理逻辑
2. **Uniform 参数**：可调节的参数（亮度、对比度、模糊半径等）

基于这个理念，我们设计了一套通用、灵活、可扩展的渲染架构。

---

## 架构层次

```
VideoRenderEngine (渲染引擎)
    │
    ├─ Renderable 1 (视频/图像/文字)
    │   └─ FilterChain (滤镜链)
    │       ├─ Filter 1 (颜色调整)
    │       ├─ Filter 2 (模糊)
    │       └─ Filter 3 (锐化)
    │
    ├─ Renderable 2
    │   └─ FilterChain
    │       └─ Filter ...
    │
    └─ Global FilterChain (全局后处理)
        ├─ Filter 1 (暗角)
        └─ Filter 2 (色差)
```

---

## 核心组件

### 1. Filter (滤镜基类)
- 定义滤镜的通用接口
- 管理 shader、uniform buffer、采样器等资源

### 2. ShaderFilter (基于 Shader 的滤镜)
- 继承自 Filter
- 接受 ShaderConfig 配置
- 自动管理 shader 编译、pipeline 创建

### 3. ShaderEffect (通用效果类) ⭐ 最重要
这是架构的核心创新！

**特点：**
- **无需为每个效果创建新类**
- **参数系统自动化**
- **支持运行时调整**
- **可序列化/反序列化**

**使用方式：**

```cpp
// 方式 1: 使用预设效果
auto colorAdjust = ShaderEffect::createColorAdjust();
colorAdjust->setParam("brightness", 0.2f);
colorAdjust->setParam("contrast", 1.3f);
colorAdjust->setParam("saturation", 1.1f);
colorAdjust->setParam("hue", 15.0f);

auto blur = ShaderEffect::createBlur(5.0f);
blur->setParam("radius", 8.0f);  // 调整模糊半径

auto sharpen = ShaderEffect::createSharpen(1.5f);
auto vignette = ShaderEffect::createVignette();
vignette->setParam("intensity", 0.6f);
vignette->setParam("radius", 0.9f);

auto chromatic = ShaderEffect::createChromaticAberration();
chromatic->setParam("amount", 0.02f);

// 方式 2: 自定义效果
ShaderConfig customConfig = /* 你的 shader 配置 */;
std::vector<ShaderParam> params = {
    {"param1", 0.5f, 0.0f, 1.0f, "参数1说明"},
    {"param2", 1.0f, 0.0f, 2.0f, "参数2说明"}
};
auto custom = std::make_unique<ShaderEffect>("MyEffect", customConfig, params);
custom->setParam("param1", 0.7f);
```

### 4. FilterChain (滤镜链)
- 管理多个滤镜的串联
- 自动创建中间渲染纹理（ping-pong buffers）
- 按顺序应用滤镜

### 5. VideoRenderEngine (渲染引擎)
- 管理多个可渲染对象
- 支持层级排序
- 每个对象独立的滤镜链
- 全局后处理滤镜链

---

## 完整使用示例

```cpp
#include <clipengine/render/VideoRenderEngine.h>
#include <clipengine/render/VideoRenderer.h>
#include <clipengine/render/ShaderEffect.h>

// 1. 创建渲染引擎
VideoRenderEngine engine;
engine.initialize(device, format, 1920, 1080);

// 2. 添加视频渲染器
auto video = std::make_unique<VideoRenderer>();
video->setViewport(0.0f, 0.0f, 1.0f, 1.0f);
video->setLayer(0);
size_t videoIdx = engine.addRenderable(std::move(video));

// 3. 为视频添加滤镜链
FilterChain* videoFilters = engine.getRenderableFilterChain(videoIdx);

// 添加颜色调整
auto colorAdjust = ShaderEffect::createColorAdjust();
colorAdjust->setParam("brightness", 0.1f);
colorAdjust->setParam("contrast", 1.2f);
videoFilters->addFilter(std::move(colorAdjust));

// 添加锐化
auto sharpen = ShaderEffect::createSharpen(1.0f);
videoFilters->addFilter(std::move(sharpen));

// 4. 添加全局后处理
auto vignette = ShaderEffect::createVignette();
vignette->setParam("intensity", 0.5f);
engine.getGlobalFilterChain().addFilter(std::move(vignette));

// 5. 渲染循环
while (running) {
    // 动态调整参数
    if (auto* filter = dynamic_cast<ShaderEffect*>(videoFilters->getFilter(0))) {
        filter->setParam("brightness", getBrightnessFromUI());
    }

    engine.update(deltaTime);
    engine.render(outputView);
}
```

---

## 内置效果预设

| 效果 | 工厂方法 | 参数 |
|------|----------|------|
| 颜色调整 | `createColorAdjust()` | brightness, contrast, saturation, hue |
| 高斯模糊 | `createBlur(radius)` | radius |
| 锐化 | `createSharpen(amount)` | amount |
| 暗角 | `createVignette()` | intensity, radius |
| 色差 | `createChromaticAberration()` | amount |

---

## 扩展新效果

### 方法 1: 使用 ShaderEffect（推荐）

```cpp
ShaderConfig myEffectConfig;
myEffectConfig.name = "My Cool Effect";
myEffectConfig.vertexShaderSource = R"(...)";
myEffectConfig.fragmentShaderSource = R"(
    @group(0) @binding(0) var mySampler : sampler;
    @group(0) @binding(1) var inputTexture : texture_2d<f32>;
    @group(0) @binding(2) var<uniform> params : vec4f;

    @fragment
    fn fs(input : VertexOutput) -> @location(0) vec4f {
        // 使用 params.x, params.y 等参数
        // 处理纹理
        return result;
    }
)";
myEffectConfig.bindings = { /* sampler, texture, uniform */ };
myEffectConfig.vertexAttributes = { /* pos, uv */ };

std::vector<ShaderParam> params = {
    {"strength", 1.0f, 0.0f, 5.0f, "Effect strength"}
};

auto effect = std::make_unique<ShaderEffect>("MyCoolEffect", myEffectConfig, params);
```

### 方法 2: 继承 Filter（仅需要特殊逻辑时）

```cpp
class MyCustomFilter : public Filter {
    // 仅在需要特殊初始化逻辑、多 pass 渲染等情况下使用
};
```

---

## 架构优势

✅ **简单** - 大多数效果只需定义 shader + 参数，无需写 C++ 类
✅ **灵活** - 运行时动态调整参数、添加/删除滤镜
✅ **高效** - Ping-pong 缓冲减少纹理拷贝
✅ **可扩展** - 轻松添加新效果
✅ **统一** - 所有效果使用相同的接口
✅ **可序列化** - 参数系统便于保存/加载配置

---

## 性能优化建议

1. **滤镜顺序**：先应用廉价效果（颜色调整），后应用昂贵效果（模糊）
2. **合并 Uniform**：相似效果可以合并到一个 shader 中
3. **避免不必要的中间纹理**：使用 `FilterChain` 自动管理
4. **禁用未使用的滤镜**：`filter->setEnabled(false)`

---

## 文件结构

```
src/clipengine/render/
├── Filter.h/cpp              # 滤镜基类
├── ShaderFilter              # 内嵌在 Filter.h 中
├── ShaderEffect.h/cpp        # 通用效果类 ⭐
├── FilterChain.h/cpp         # 滤镜链管理
├── VideoRenderEngine.h/cpp   # 渲染引擎
├── TextureRenderer.h/cpp     # 纹理渲染器
├── VideoRenderer.h/cpp       # 视频渲染器
└── ShaderConfig.h/cpp        # Shader 配置
```

---

## 总结

这个架构完美体现了"滤镜 = Shader + 参数"的理念。通过 `ShaderEffect`，你可以：

- 🚀 **快速开发**：新效果只需写 shader，无需写 C++ 类
- 🎨 **灵活调整**：所有参数实时可调
- 📦 **易于维护**：统一的接口和参数系统
- 🔧 **便于扩展**：添加新效果轻而易举

**Remember**: 99% 的效果都可以用 `ShaderEffect` 实现，只有极少数需要特殊逻辑的才需要继承 `Filter`。
