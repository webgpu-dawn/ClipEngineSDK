# SDK Demo - ClipEngine 综合示例

这是一个完整的 ClipEngine SDK 使用示例，展示了视频处理的完整流程。

**位置**: `ClipEngineSDK/samples/sdk_demo/`

## 功能演示

这个 demo 演示了 ClipEngine 的核心功能：

### ✅ 视频处理完整流程
- **视频解码** - 使用 FFmpeg 解码视频文件
- **实时渲染** - 使用 CompositionEngine 进行实时视频渲染
- **效果应用** - 色彩调整、曝光、增益等后期效果
- **视频导出** - 将处理后的视频导出为文件
- **交互控制** - 使用 InputSystem 进行交互式控制

### ✅ 使用的核心组件
- `CompositionEngine` - 合成引擎
- `VideoRenderer` - 视频渲染器
- `VideoExporter` - 视频导出器
- `ShaderEffect` - 着色器效果
- `InputSystem` - 输入系统
- `DebugWindow` - 调试窗口（ImGui）

### ✅ 支持的渲染模式
- **Equirectangular** - 等距矩形投影（全景视频标准格式）
- **Perspective** - 透视投影（小行星效果）
- **其他投影模式** - 可扩展

## 构建和运行

### 前置要求

- CMake 3.13+
- C++20 编译器
- ClipEngine SDK (Debug 或 Release)
- FFmpeg 库
- GLFW 3.x
- ImGui（通过 SDK 提供）

### 部署 SDK

首先需要构建并部署 ClipEngine SDK：

```bash
# 在 ClipEngineSDK 根目录
mkdir build
cd build
cmake ..
cmake --build . --config Debug
cmake --install . --config Debug
```

这会将 SDK 安装到 `output/ClipEngineSDK-Debug/` 目录。

### 准备依赖

将以下依赖复制到 `samples/sdk_demo/deps/` 目录：

```
samples/sdk_demo/deps/
├── ClipEngineSDK-Debug/      # 从 output/ 复制
├── ClipEngineSDK-Release/    # 从 output/ 复制（可选）
├── ffmpeg_x64-windows/       # FFmpeg 库
├── glfw/                     # GLFW 库
└── imgui_x64-windows/        # ImGui 库
```

或者使用提供的部署脚本：

```bash
# Windows
deploy_example.bat

# PowerShell
.\deploy_example.ps1
```

### 构建示例

```bash
cd ClipEngineSDK/samples/sdk_demo
mkdir build
cd build
cmake ..
cmake --build . --config Debug
```

### 运行

```bash
# Windows
.\Debug\sdk_demo.exe

# Linux/macOS
./sdk_demo
```

## 使用说明

### 视频文件

程序会尝试加载视频文件。你可以修改 `Application.cpp` 中的视频路径：

```cpp
// 在 setupScene() 中
videoSource_ = std::make_unique<VideoSource>("path/to/your/video.mp4");
```

### 交互控制

运行程序后可以使用以下控制：

| 操作 | 功能 |
|------|------|
| **鼠标拖拽** | 旋转视角（在透视模式下）|
| **鼠标滚轮** | 缩放 |
| **数字键 1-5** | 切换渲染模式 |
| **调试窗口** | 调整效果参数（曝光、增益、饱和度等）|
| **ESC** | 退出程序 |

### 导出视频

程序演示了如何导出处理后的视频：

```cpp
// 导出为 MP4
videoExporter->startExport("output.mp4", width, height, 30.0);

// 导出每一帧
for (each frame) {
    videoExporter->exportFrame(texture);
}

// 完成导出
videoExporter->finishExport();
```

## 代码结构

```
samples/sdk_demo/
├── main.cpp           # 程序入口
├── Application.h/cpp  # 主应用程序类
├── VideoSource.h/cpp  # 视频源管理（解码）
├── Decoder.h/cpp      # FFmpeg 解码器封装
├── CMakeLists.txt     # 构建配置
└── deps/              # 依赖库
    ├── ClipEngineSDK-Debug/
    ├── ffmpeg_x64-windows/
    └── ...
```

### 核心代码

#### 初始化引擎

```cpp
CompositionEngine engine;
engine.initialize(deviceConfig);

// 创建视频渲染器
VideoRenderer* videoRenderer = engine.createLayer<VideoRenderer>();
videoRenderer->setRenderMode(VideoRenderer::RenderMode::Equirectangular);
```

#### 加载视频

```cpp
VideoSource videoSource("video.mp4");
videoSource.initialize();

// 获取视频帧
VideoFrame frame = videoSource.getNextFrame();
videoRenderer->updateTexture(frame.data, frame.width, frame.height);
```

#### 应用效果

```cpp
ShaderEffect* colorAdjust = engine.createEffect("color_adjust");
colorAdjust->setParameter("brightness", 0.2f);
colorAdjust->setParameter("saturation", 1.2f);
videoRenderer->addEffect(colorAdjust);
```

#### 渲染和导出

```cpp
// 渲染到屏幕
engine.render();

// 导出帧
VideoExporter exporter;
exporter.startExport("output.mp4", 1920, 1080, 30.0);
exporter.exportFrame(engine.captureFrame());
exporter.finishExport();
```

## 学习要点

通过这个示例，您可以学习：

1. **CompositionEngine 使用** - 引擎的初始化、配置和渲染流程
2. **视频解码集成** - 如何集成 FFmpeg 进行视频解码
3. **图层管理** - VideoRenderer 的使用和配置
4. **效果系统** - 如何创建和应用着色器效果
5. **视频导出** - 完整的视频导出流程
6. **输入处理** - InputSystem 的实际应用
7. **调试工具** - DebugWindow 的集成和使用

## 故障排除

### 编译错误

**问题**: 找不到 ClipEngine
```
Solution: 确保已经构建并安装了 ClipEngine SDK
cmake --install . --config Debug
```

**问题**: 找不到 FFmpeg
```
Solution: 确保 deps/ffmpeg_x64-windows 目录存在
检查 CMakeLists.txt 中的路径配置
```

### 运行时问题

**问题**: 缺少 DLL
```
Solution: DLL 会自动复制到输出目录
检查 build/Debug/ 目录下是否有：
- webgpu_dawn.dll
- d3dcompiler_47.dll
- vulkan-1.dll
- FFmpeg DLL 文件
```

**问题**: 视频加载失败
```
Solution: 检查视频文件路径
确保使用支持的视频格式（MP4, H.264）
```

**问题**: 找不到 shader 文件
```
Solution: Shader 文件会自动复制到 build/Debug/shaders
检查该目录是否存在
```

## 性能建议

1. **使用 Release 构建** - 性能比 Debug 提升显著
   ```bash
   cmake --build . --config Release
   ```

2. **调整视频分辨率** - 降低分辨率可以提高性能

3. **减少效果数量** - 过多的后期效果会影响性能

4. **优化导出设置** - 根据需要调整导出质量和帧率

## 扩展建议

基于此示例，您可以尝试：

1. **添加更多效果** - 实现自定义着色器效果
2. **多视频源** - 支持多个视频图层的合成
3. **音频处理** - 集成音频解码和同步
4. **实时预览** - 优化渲染性能以支持实时预览
5. **UI 增强** - 添加更丰富的用户界面

## 相关文档

- [ClipEngine API 文档](../../src/clipengine/clipengine.h)
- [InputSystem 文档](../../src/clipengine/input/README.md)
- [VideoExporter 使用指南](../../src/clipengine/export/VideoExporter.h)

## 反馈

如果您发现任何问题或有改进建议，欢迎提交 Issue 或 Pull Request。

---

**Last Updated**: 2025-10-31
**ClipEngineSDK Version**: v1.0
