# ClipEngine Shader Effects Configuration System

这个目录包含基于配置文件的 shader 特效系统，允许你通过简单的文本配置文件来定义特效，而不需要重新编译代码。

## 目录结构

```
effects/
├── README.md                      # 本文档
├── *.effect                       # 特效配置文件
└── shaders/
    ├── fullscreen.vert.wgsl      # 通用全屏顶点着色器
    └── *.frag.wgsl               # 各种片段着色器
```

## 配置文件格式 (.effect)

配置文件使用类似 INI 的简单格式：

```ini
# 注释以 # 开头

[Effect]
name = EffectName                  # 特效名称
vertexShader = path/to/vertex.wgsl # 顶点着色器路径
fragmentShader = path/to/frag.wgsl # 片段着色器路径
inputBinding = true                # 是否启用输入绑定（用于时间、鼠标等）

[Parameter]
name = paramName                   # 参数名称
type = float                       # 参数类型: float, int, bool, vec2, vec3, vec4
default = 1.0                      # 默认值
min = 0.0                          # 最小值
max = 2.0                          # 最大值
description = Parameter description # 参数描述

# 可以有多个 [Parameter] 部分
[Parameter]
name = anotherParam
type = float
default = 0.5
min = 0.0
max = 1.0
description = Another parameter
```

## 示例：老电影特效

`vintage_film.effect` 演示了完整的特效配置：

- **Sepia Tone**: 复古棕褐色调
- **Film Grain**: 动态胶片颗粒噪点
- **Scratches**: 随机垂直划痕
- **Vignette**: 边缘暗角效果

### Shader 文件组织

#### 顶点着色器 (`fullscreen.vert.wgsl`)
通用的全屏四边形顶点着色器，被大多数后处理特效复用。

#### 片段着色器 (`vintage_film.frag.wgsl`)
具体的特效实现，包含：
- 统一变量绑定 (@binding)
- 辅助函数（random, filmGrain, applySepia, scratches）
- 主片段着色器函数

## 如何创建新特效

### 1. 编写 Shader

创建片段着色器文件 `effects/shaders/my_effect.frag.wgsl`:

```wgsl
@group(0) @binding(0) var mySampler : sampler;
@group(0) @binding(1) var inputTexture : texture_2d<f32>;
@group(0) @binding(2) var<uniform> params : vec4f;

struct VertexOutput {
    @builtin(position) pos : vec4f,
    @location(0) uv : vec2f
};

@fragment
fn fs(input : VertexOutput) -> @location(0) vec4f {
    let intensity = params.x;
    let color = textureSample(inputTexture, mySampler, input.uv);
    // 你的特效逻辑
    return color;
}
```

### 2. 创建配置文件

创建 `effects/my_effect.effect`:

```ini
[Effect]
name = MyEffect
vertexShader = effects/shaders/fullscreen.vert.wgsl
fragmentShader = effects/shaders/my_effect.frag.wgsl
inputBinding = false

[Parameter]
name = intensity
type = float
default = 1.0
min = 0.0
max = 2.0
description = Effect intensity
```

### 3. 在代码中使用

未来实现 EffectLoader 后，使用方式将会是：

```cpp
// 从配置文件加载特效
auto effect = ShaderEffect::loadFromFile("effects/my_effect.effect");

// 自定义参数（可选）
effect->setParam("intensity", 1.5f);

// 添加到滤镜链
engine.getGlobalFilterChain().addFilter(std::move(effect));
```

## 参数类型

### 支持的类型

- `float`: 单精度浮点数
- `int`: 整数
- `bool`: 布尔值
- `vec2`: 2D 向量 (2个float)
- `vec3`: 3D 向量 (3个float)
- `vec4`: 4D 向量 (4个float)

### 自动填充参数

以下参数名称会被自动填充（需要 `inputBinding = true`）：

- `iTime`: 经过的时间（秒）
- `iTimeDelta`: 帧时间差
- `iFrame`: 帧计数
- `iResolution`: 分辨率 (vec3: width, height, aspectRatio)
- `iMouse`: 鼠标状态 (vec4: x, y, clickX, clickY)

## Shader 绑定规范

所有特效 shader 必须遵循统一的绑定布局：

```wgsl
@group(0) @binding(0) var mySampler : sampler;
@group(0) @binding(1) var inputTexture : texture_2d<f32>;
@group(0) @binding(2) var<uniform> params : vec4f;  // 第一组参数 (最多4个)
@group(0) @binding(3) var<uniform> params2 : vec4f; // 第二组参数 (可选)
// 更多参数组...
```

### 参数打包规则

参数按**字母顺序**打包到 vec4 中：

- 如果有 5 个参数（按字母排序）：
  - `params.xyzw` = 参数 1-4
  - `params2.x` = 参数 5

## 扩展性设计

这个系统的优势：

1. **无需重新编译**: 修改特效只需编辑配置和 shader 文件
2. **易于分享**: 配置文件 + shader 文件即可分享特效
3. **可视化编辑**: 未来可以实现GUI编辑器
4. **版本控制友好**: 文本文件便于 git 管理
5. **Hot Reload**: 可以实现运行时重新加载特效

## 下一步实现

为了完全启用这个系统，需要实现：

1. **EffectLoader 类**: 解析 .effect 文件
2. **ShaderEffect::loadFromFile()**: 从配置加载特效
3. **Shader 文件读取**: 从磁盘读取 .wgsl 文件
4. **参数验证**: 检查参数类型和范围
5. **错误处理**: 友好的错误消息

## 贡献特效

欢迎贡献新的特效！只需：

1. 在 `effects/shaders/` 创建 shader 文件
2. 在 `effects/` 创建配置文件
3. 测试特效
4. 提交 Pull Request

## 示例特效库

计划支持的特效：

- ✅ Vintage Film (老电影)
- 📋 Gaussian Blur (高斯模糊)
- 📋 Chromatic Aberration (色差)
- 📋 Vignette (暗角)
- 📋 Film Grain (胶片颗粒)
- 📋 Color Grading (调色)
- 📋 Glitch (故障艺术)
- 📋 CRT Monitor (CRT显示器)
