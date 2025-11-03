# ClipEngineSDK Directory Structure

本文档说明 ClipEngineSDK 的目录结构和各部分用途。

## 📁 根目录结构

```
ClipEngineSDK/
├── src/                    # 源代码
│   └── clipengine/         # ClipEngine 核心库
│       ├── core/           # 核心渲染系统
│       ├── input/          # 输入系统
│       ├── effects/        # 效果系统
│       ├── layers/         # 图层系统
│       ├── export/         # 导出系统
│       └── shaders/        # 着色器
│
├── samples/                # 📚 示例程序（各种功能演示）
│   ├── input_system_demo/  # InputSystem 使用示例
│   │   ├── main.cpp
│   │   ├── CMakeLists.txt
│   │   ├── README.md
│   │   ├── QUICKSTART.md
│   │   ├── build.bat
│   │   ├── build.sh
│   │   ├── run.bat
│   │   └── run.sh
│   │
│   └── sdk_demo/           # SDK 综合示例（视频处理完整示例）
│       ├── Application.cpp
│       ├── VideoSource.cpp
│       ├── Decoder.cpp
│       ├── main.cpp
│       ├── CMakeLists.txt
│       └── deps/           # 示例依赖（SDK 包、FFmpeg 等）
│
├── assets/                 # 资源文件
├── deps/                   # 依赖库（GLFW, Dawn, etc.）
├── build/                  # 构建输出
├── output/                 # SDK 安装输出
└── cmake/                  # CMake 配置文件
```

## 📂 目录说明

### 📚 samples/ - 示例程序
**用途**: 存放各种示例程序，演示如何使用 ClipEngine 的各个功能模块。

**特点**:
- 从简单到复杂的示例
- 易于理解和学习
- 独立的构建系统
- 详细的文档说明

**当前包含**:

1. **`input_system_demo/`** - InputSystem 使用示例
   - 演示窗口系统无关的输入处理
   - 鼠标、键盘、滚轮事件
   - 事件监听器模式
   - 平台无关的键码定义

2. **`sdk_demo/`** - SDK 综合示例
   - 完整的视频处理应用
   - 包含视频解码、渲染、效果应用、导出等功能
   - 使用 FFmpeg 进行视频解码
   - 演示 CompositionEngine、VideoRenderer、VideoExporter 等核心功能
   - 带调试窗口（使用 ImGui）

**未来可添加**:
- `basic_rendering/` - 基础渲染示例
- `video_effects/` - 视频效果应用示例
- `layer_composition/` - 图层合成示例
- `custom_shader/` - 自定义着色器示例

## 🔨 构建指南

### 示例程序构建

#### InputSystem Demo (samples/input_system_demo)
简单的输入系统示例，快速启动：
```bash
cd ClipEngineSDK/samples/input_system_demo
build.bat           # Windows
./build.sh          # Linux/macOS
run.bat             # 运行 (Windows)
./run.sh            # 运行 (Linux/macOS)
```

#### SDK Demo (samples/sdk_demo)
完整的 SDK 功能演示，需要预先部署 SDK：
```bash
cd ClipEngineSDK/samples/sdk_demo
mkdir build && cd build
cmake ..
cmake --build . --config Debug

# 运行
./Debug/sdk_demo.exe    # Windows
./sdk_demo              # Linux/macOS
```

## 📝 添加新示例

要添加新的示例程序到 `samples/` 目录：

1. 在 `samples/` 下创建新目录，如 `samples/my_example/`
2. 添加源代码文件
3. 创建 `CMakeLists.txt`，参考现有示例：
   - 简单示例：参考 `input_system_demo/CMakeLists.txt`
   - 复杂示例：参考 `sdk_demo/CMakeLists.txt`
4. 添加 `README.md` 说明示例的用途和使用方法
5. 可选：添加构建脚本 `build.bat` 和 `build.sh`（适用于简单示例）

## 🔗 相关文档

- [InputSystem 文档](src/clipengine/input/README.md)
- [InputSystem Demo 文档](samples/input_system_demo/README.md)
- [InputSystem Demo 快速开始](samples/input_system_demo/QUICKSTART.md)

## 📊 目录演进

### 之前的结构 ❌
```
example/                    # 混合了示例和测试
├── input_system_demo/      # 简单示例
├── Application.cpp         # 复杂测试
├── VideoSource.cpp         # 复杂测试
└── ...

examples/                   # 混合了示例和效果配置文件
├── color_adjust.json
└── custom_shader.wgsl
```

### 现在的结构 ✅
```
samples/                    # 所有示例程序
├── input_system_demo/      # 简单示例
└── sdk_demo/               # 完整示例
```

## 🎯 设计原则

1. **统一的示例目录**: 所有示例程序都在 `samples/` 下
2. **清晰的命名**: 目录名称清楚表明其用途和复杂度
3. **独立构建**: 每个示例都可以独立构建
4. **完整的文档**: 复杂示例提供详细的说明文档
5. **易于扩展**: 可以轻松添加新的示例
6. **分层复杂度**: 从简单到复杂的示例，适合不同学习阶段

---

**最后更新**: 2025-10-31
**版本**: ClipEngineSDK v1.0
