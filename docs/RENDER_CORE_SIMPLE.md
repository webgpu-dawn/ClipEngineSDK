# ClipEngine - Simplified Render Core

> 🎯 **核心功能**: 图层 + 变换 + 滤镜链的渲染管线

---

## 📊 架构总览

![Simple Render Core](images/render_core_simple.svg)

---

## 🏗️ 三大核心模块

### 1️⃣ Layer System (图层系统)
**职责**: 管理和渲染不同类型的图层

```
Layer System
├── Layer Types (图层类型)
│   ├── ImageLayer (图片图层)
│   │   ├── 输入: 图片文件路径
│   │   ├── 处理: stb_image 解码 → GPU 纹理
│   │   └── 输出: wgpu::TextureView
│   │
│   ├── VideoLayer (视频图层)
│   │   ├── 输入: 视频文件 + 时间戳
│   │   ├── 处理: FFmpeg 解码 → GPU 纹理
│   │   └── 输出: wgpu::TextureView
│   │
│   ├── SolidLayer (纯色图层)
│   │   ├── 输入: 颜色值 (RGBA)
│   │   ├── 处理: GPU 生成纯色纹理
│   │   └── 输出: wgpu::TextureView
│   │
│   └── TextLayer (文字图层)
│       ├── 输入: 文字 + 字体参数
│       ├── 处理: FreeType 光栅化 → GPU 纹理
│       └── 输出: wgpu::TextureView
│
├── Layer Properties (图层属性)
│   ├── Opacity (不透明度: 0.0 ~ 1.0)
│   ├── Blend Mode (混合模式: Normal/Add/Multiply...)
│   ├── Visible (可见性: true/false)
│   └── Time Range (时间范围: start ~ end)
│
└── Layer Stack (图层栈)
    ├── Z-Order (层级顺序)
    ├── Parent-Child (父子关系)
    └── Layer Composition (图层合成)
```

**输入**: 图层配置 (类型 + 源数据 + 属性)
**输出**: wgpu::TextureView (GPU 纹理视图)

---

### 2️⃣ Transform System (变换系统)
**职责**: 对图层应用 2D/3D 变换

```
Transform System
├── 2D Transform (2D 变换)
│   ├── Position (位置: x, y)
│   ├── Rotation (旋转: angle)
│   ├── Scale (缩放: sx, sy)
│   ├── Anchor Point (锚点: ax, ay)
│   └── Opacity (不透明度: alpha)
│
├── 3D Transform (3D 变换, 可选)
│   ├── Position 3D (位置: x, y, z)
│   ├── Rotation 3D (旋转: rx, ry, rz)
│   ├── Scale 3D (缩放: sx, sy, sz)
│   └── Camera (相机参数)
│
├── Transform Matrix (变换矩阵)
│   ├── Matrix Construction (矩阵构建)
│   │   └── T * R * S (平移 * 旋转 * 缩放)
│   ├── Matrix Combination (矩阵组合)
│   │   └── Parent * Child (父级 * 子级)
│   └── GPU Upload (GPU 上传)
│       └── Uniform Buffer
│
└── Animation Support (动画支持)
    ├── Keyframes (关键帧)
    ├── Interpolation (插值: Linear/Bezier/Ease)
    └── Time Evaluation (时间求值)
```

**输入**: wgpu::TextureView + Transform 参数
**输出**: 变换后的渲染结果

---

### 3️⃣ Filter Chain (滤镜链)
**职责**: 对图层应用一系列滤镜效果

```
Filter Chain
├── Filter Categories (滤镜分类)
│   ├── Color Adjustments (色彩调整)
│   │   ├── Brightness/Contrast (亮度/对比度)
│   │   ├── Saturation/Hue (饱和度/色相)
│   │   ├── Curves (曲线)
│   │   ├── Levels (色阶)
│   │   └── Color Balance (色彩平衡)
│   │
│   ├── Blur & Sharpen (模糊与锐化)
│   │   ├── Gaussian Blur (高斯模糊)
│   │   ├── Box Blur (方框模糊)
│   │   ├── Motion Blur (运动模糊)
│   │   └── Sharpen (锐化)
│   │
│   ├── Distortion (扭曲)
│   │   ├── Warp (变形)
│   │   ├── Bulge (凸起)
│   │   ├── Ripple (波纹)
│   │   └── Lens Distortion (镜头畸变)
│   │
│   ├── Stylize (风格化)
│   │   ├── Glow (发光)
│   │   ├── Edge Detect (边缘检测)
│   │   ├── Posterize (色调分离)
│   │   └── Pixelate (像素化)
│   │
│   └── Custom Filters (自定义滤镜)
│       ├── GLSL/WGSL Shaders
│       └── Compute Shaders
│
├── Filter Pipeline (滤镜管线)
│   ├── Filter Order (滤镜顺序)
│   ├── Filter Parameters (滤镜参数)
│   ├── Parameter Animation (参数动画)
│   └── Enable/Disable (启用/禁用)
│
└── GPU Execution (GPU 执行)
    ├── Render Pass (渲染通道)
    ├── Ping-Pong Buffers (乒乓缓冲)
    │   └── A → B → A → B (交替渲染)
    └── Multi-pass Rendering (多通道渲染)
```

**输入**: wgpu::TextureView + Filter 参数列表
**输出**: 应用滤镜后的纹理

---

## 🔄 完整渲染流程

```
1. Layer 图层
   ├─ 加载源数据 (图片/视频/纯色/文字)
   └─ 输出: GPU TextureView

   ↓

2. Transform 变换
   ├─ 构建变换矩阵 (T * R * S)
   ├─ 应用到顶点着色器
   └─ 输出: 变换后的渲染结果

   ↓

3. Filter Chain 滤镜链
   ├─ 逐个应用滤镜 (Filter 1 → 2 → N)
   ├─ 使用 Ping-Pong 缓冲
   └─ 输出: 最终渲染结果

   ↓

4. 合成输出
   └─ 如果有多个图层，合成到 FrameBuffer
```

---

## 📋 数据结构设计

### Layer (图层基类)

```cpp
class Layer {
public:
    // 基本属性
    LayerID id;
    string name;
    LayerType type;  // Image/Video/Solid/Text

    // 可见性
    bool visible;
    float opacity;  // 0.0 ~ 1.0
    BlendMode blendMode;

    // 时间范围
    float startTime;
    float duration;

    // 变换
    Transform2D transform;

    // 滤镜链
    vector<Filter*> filters;

    // 渲染
    virtual wgpu::TextureView render(float time) = 0;
};
```

### Transform2D (2D 变换)

```cpp
struct Transform2D {
    // 位置
    glm::vec2 position = {0.0f, 0.0f};

    // 旋转 (角度)
    float rotation = 0.0f;

    // 缩放
    glm::vec2 scale = {1.0f, 1.0f};

    // 锚点
    glm::vec2 anchor = {0.5f, 0.5f};  // 中心点

    // 构建变换矩阵
    glm::mat4 toMatrix() const {
        glm::mat4 mat = glm::mat4(1.0f);

        // 1. 平移到锚点
        mat = glm::translate(mat, glm::vec3(-anchor, 0.0f));

        // 2. 缩放
        mat = glm::scale(mat, glm::vec3(scale, 1.0f));

        // 3. 旋转
        mat = glm::rotate(mat, glm::radians(rotation), glm::vec3(0, 0, 1));

        // 4. 平移到目标位置
        mat = glm::translate(mat, glm::vec3(position, 0.0f));

        return mat;
    }
};
```

### Filter (滤镜基类)

```cpp
class Filter {
public:
    // 基本信息
    string name;
    FilterType type;
    bool enabled = true;

    // 参数
    map<string, Parameter> parameters;

    // 渲染
    virtual wgpu::TextureView apply(
        wgpu::TextureView input,
        wgpu::Device device,
        float time
    ) = 0;

    // GPU Shader
    virtual wgpu::ShaderModule getShader() = 0;
};
```

---

## 🎨 渲染示例

### 单图层渲染

```cpp
// 1. 创建图层
auto imageLayer = new ImageLayer();
imageLayer->loadImage("input.png");

// 2. 设置变换
imageLayer->transform.position = {100.0f, 200.0f};
imageLayer->transform.rotation = 45.0f;
imageLayer->transform.scale = {1.5f, 1.5f};

// 3. 添加滤镜
auto blur = new GaussianBlur();
blur->setRadius(10.0f);
imageLayer->addFilter(blur);

auto colorAdjust = new ColorAdjustment();
colorAdjust->setBrightness(1.2f);
colorAdjust->setContrast(1.1f);
imageLayer->addFilter(colorAdjust);

// 4. 渲染
wgpu::TextureView result = imageLayer->render(0.0f);
```

### 多图层合成

```cpp
// 1. 创建多个图层
auto background = new SolidLayer({0, 0, 0, 1});  // 黑色背景
auto image1 = new ImageLayer("photo1.png");
auto image2 = new ImageLayer("photo2.png");
auto text = new TextLayer("Hello World");

// 2. 设置各层变换和滤镜
image1->transform.position = {100, 100};
image1->addFilter(new GaussianBlur(5.0f));

image2->transform.position = {300, 200};
image2->transform.rotation = 30.0f;
image2->addFilter(new Glow(0.5f));

text->transform.position = {400, 500};

// 3. 创建图层栈
LayerStack stack;
stack.addLayer(background);
stack.addLayer(image1);
stack.addLayer(image2);
stack.addLayer(text);

// 4. 合成渲染
Compositor compositor;
wgpu::TextureView finalResult = compositor.render(stack, 0.0f);
```

---

## ⚡ 性能优化

### 1. 滤镜缓存
```cpp
// 如果滤镜参数没变，复用上次结果
if (!filter->isDirty()) {
    return cachedTexture;
}
```

### 2. 图层缓存
```cpp
// 如果图层内容、变换、滤镜都没变，复用缓存
if (!layer->isDirty() && !layer->transform.isDirty() && !layer->filters.isDirty()) {
    return cachedLayerTexture;
}
```

### 3. GPU 并行
```cpp
// 多个独立图层可并行渲染
#pragma omp parallel for
for (int i = 0; i < layers.size(); i++) {
    layerResults[i] = layers[i]->render(time);
}
```

---

## 请确认以下架构：

### ✅ 三大核心模块
1. **Layer System** - 图层系统 (Image/Video/Solid/Text)
2. **Transform System** - 变换系统 (Position/Rotation/Scale)
3. **Filter Chain** - 滤镜链 (Color/Blur/Distortion/Stylize)

### ✅ 渲染流程
```
Layer → Transform → Filter Chain → Output
```

### ✅ 数据结构
- `Layer` 基类
- `Transform2D` 结构
- `Filter` 基类

**确认后我将创建配套的架构图和详细设计！** 🚀
