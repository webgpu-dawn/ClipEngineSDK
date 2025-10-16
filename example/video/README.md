# ClipEngine Video Example

这个示例展示如何使用ClipEngine SDK渲染视频。

## 构建 ClipEngine SDK

首先，构建 Debug 和 Release 版本的 ClipEngine SDK：

```bash
cd D:\TestDawn

# 配置项目
cmake -B build -S .

# 构建 Debug SDK
cmake --build build --config Debug --target install_sdk

# 构建 Release SDK
cmake --build build --config Release --target install_sdk
```

这将创建两个 SDK 目录：
- `D:\TestDawn\output\ClipEngineSDK-Debug\` - Debug 库和头文件
- `D:\TestDawn\output\ClipEngineSDK-Release\` - Release 库和头文件

## 构建 Video Example

由于 Windows 上 Debug 和 Release 的运行时库不兼容，需要为每个配置创建单独的构建目录：

### Debug 构建

```bash
cd D:\TestDawn\example\video

# 创建 Debug 构建目录
cmake -B build-debug -S .

# 构建 Debug 版本
cmake --build build-debug --config Debug

# 运行
.\build-debug\Debug\video.exe
```

### Release 构建

```bash
cd D:\TestDawn\example\video

# 创建 Release 构建目录
cmake -B build-release -S .

# 先配置为使用 Release SDK
cmake -B build-release -S . -DCMAKE_PREFIX_PATH="../../output/ClipEngineSDK-Release"

# 构建 Release 版本
cmake --build build-release --config Release

# 运行
.\build-release\Release\video.exe
```

## SDK 目录结构

每个 SDK 目录包含：
```
ClipEngineSDK-{Debug|Release}/
├── bin/
│   └── webgpu_dawn.dll         # Dawn WebGPU 动态库
├── include/
│   ├── clipengine/             # ClipEngine 头文件
│   └── dawn/                   # Dawn/WebGPU 头文件
├── lib/
│   ├── clipengine.lib          # 主库文件
│   ├── webgpu_dawn.lib         # Dawn 导入库
│   ├── glfw3.lib               # GLFW 库
│   ├── glfw3webgpu.lib         # GLFW-WebGPU 桥接库
│   └── cmake/
│       ├── ClipEngine/         # ClipEngine CMake 配置
│       └── Dawn/               # Dawn CMake 配置
└── share/
    └── clipengine/shaders/     # WGSL 着色器文件
```

## 使用 ClipEngine SDK

在你的 CMakeLists.txt 中：

```cmake
# 查找 ClipEngine SDK
find_package(ClipEngine REQUIRED)

# 链接 ClipEngine
target_link_libraries(your_target PRIVATE ClipEngine::clipengine)

# 自动复制运行时 DLL（webgpu_dawn.dll）
# 这个函数由 ClipEngineConfig.cmake 提供
clipengine_copy_dlls(your_target)
```

就这么简单！`clipengine_copy_dlls()` 函数会：
- 自动检测 SDK 配置（Debug 或 Release）
- 根据构建配置复制正确的 DLL 文件
- 支持多配置生成器（Visual Studio）

## 依赖项

- **ClipEngine SDK**: 视频渲染引擎（已包含 Dawn WebGPU 和 GLFW）
- **FFMPEG**: 视频解码（通过 vcpkg 安装）
- **vcpkg**: 包管理器（用于 FFMPEG）

所有必要的 ClipEngine DLL 文件会通过 `clipengine_copy_dlls()` 自动拷贝到输出目录。

## 重要说明

- ⚠️ **Debug 和 Release 库不可混用**：由于使用不同的 C++ 运行时库（MDd vs MD），必须确保应用程序的构建配置与 SDK 配置匹配
- SDK 独立于主项目，可以分发给其他项目使用
- 每次重新构建 ClipEngine 后，需要重新运行 `install_sdk` 目标以更新 SDK

## 使用说明

运行 video 示例：

```bash
# 确保视频文件存在
# 修改 main.cpp 中的视频路径

# Debug 版本
.\build-debug\Debug\video.exe

# Release 版本
.\build-release\Release\video.exe
```

## 项目结构

```
example/video/
├── CMakeLists.txt    # 使用 ClipEngine SDK 的 CMake 配置
├── main.cpp          # 主程序入口
├── decoder.h/cpp     # FFmpeg 视频解码器
└── README.md         # 本文件
```
