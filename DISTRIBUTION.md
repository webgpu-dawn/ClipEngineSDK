# ClipEngine SDK - Distribution Guide

## ✅ Build Status

**Last Build**: Successfully compiled on 2025-11-07
- ✅ Debug Build: Complete
- ✅ Release Build: Complete
- ✅ Samples: Verified
- ✅ Tests: Verified

## 📦 Distribution Structure

The SDK is available in two configurations:

### Debug Build
Location: `output/ClipEngineSDK-Debug/`

### Release Build
Location: `output/ClipEngineSDK-Release/`

## 📂 Directory Layout

```
ClipEngineSDK-{Debug|Release}/
├── bin/                      # Runtime DLLs
│   ├── webgpu_dawn.dll      # WebGPU implementation
│   ├── d3dcompiler_47.dll   # DirectX shader compiler
│   └── vulkan-1.dll         # Vulkan loader
│
├── lib/                      # Static libraries
│   ├── clipengine.lib       # Core engine library
│   ├── clipengine_debug.lib # Debug window support
│   ├── imgui_core.lib       # ImGui core
│   ├── imgui_backends.lib   # ImGui backends
│   ├── glfw3.lib            # GLFW windowing
│   └── cmake/               # CMake configuration files
│       ├── ClipEngine/
│       └── Dawn/
│
├── include/                  # Header files
│   ├── clipengine/          # ClipEngine headers
│   │   ├── clipengine.h     # Main header (include this)
│   │   ├── core/            # Core engine
│   │   ├── layers/          # VideoLayer, ImageLayer
│   │   ├── effects/         # ShaderEffect, filters
│   │   ├── export/          # VideoExporter
│   │   ├── input/           # InputSystem
│   │   ├── shaders/         # Shader utilities
│   │   └── utils/           # Utility functions
│   ├── dawn/                # WebGPU/Dawn headers
│   └── GLFW/                # GLFW headers
│
└── share/                    # Shared resources
    └── clipengine/
        └── shaders/         # WGSL shader files
            ├── planar_nv12.wgsl
            ├── planar_rgba.wgsl
            ├── panorama_nv12.wgsl
            ├── panorama_rgba.wgsl
            ├── little_planet_nv12.wgsl
            ├── crystal_ball_nv12.wgsl
            └── video.wgsl
```

## 🚀 How to Use the SDK

### 1. Copy SDK to Your Project

Copy the entire `ClipEngineSDK-{Debug|Release}` folder to your project.

### 2. CMake Integration

```cmake
# Set SDK path
list(APPEND CMAKE_PREFIX_PATH "path/to/ClipEngineSDK-Debug")

# Find package
find_package(ClipEngine REQUIRED)

# Link to your target
target_link_libraries(YourApp PRIVATE ClipEngine::clipengine)

# Optional: Add debug window support
if(TARGET ClipEngine::clipengine_debug)
    target_link_libraries(YourApp PRIVATE ClipEngine::clipengine_debug)
    target_compile_definitions(YourApp PRIVATE CLIPENGINE_DEBUG_WINDOW_ENABLED)
endif()
```

### 3. Include in Your Code

```cpp
#include <clipengine/clipengine.h>

int main() {
    CompositionEngine engine;
    engine.initialize(device, format, width, height);

    // Create video layer
    auto video = std::make_unique<VideoLayer>();
    engine.addLayer(std::move(video));

    // Render
    engine.render(outputView);
}
```

## 📋 Sample Application

A complete sample application is available in `samples/`:

### Build the Sample

```bash
cd samples
cmake -S . -B build -DCMAKE_PREFIX_PATH="path/to/ClipEngineSDK-Debug"
cmake --build build --config Debug
```

### Run the Sample

```bash
cd build/Debug/samples
./Sample_sdk_demo.exe
```

The sample demonstrates:
- ✅ 360° panorama video rendering
- ✅ Multiple render modes (Planar, Panorama, Little Planet, Crystal Ball)
- ✅ Real-time color adjustments (brightness, contrast, exposure, gain)
- ✅ Image overlay support
- ✅ Video export functionality
- ✅ Interactive camera control

### Sample Controls

**Render Modes**:
- `1` - Planar mode
- `2` - Panorama (360°) mode
- `3` - Little Planet mode
- `4` - Crystal Ball mode

**Color Adjustments**:
- `Q/W` - Brightness +/-
- `A/S` - Contrast +/-
- `Z/X` - Exposure +/-
- `C/V` - Gain +/-
- `R` - Reset all

**Other**:
- `L` - Load image overlay
- `I` - Toggle image visibility
- `E` - Export video
- `ESC` - Exit
- `Mouse drag` - Rotate camera
- `Mouse wheel` - Zoom

## 📦 Required Runtime Files

When distributing your application, include these DLLs:

### From SDK bin/ folder:
- `webgpu_dawn.dll` - WebGPU implementation
- `d3dcompiler_47.dll` - DirectX shader compiler
- `vulkan-1.dll` - Vulkan loader

### From SDK share/ folder:
- `shaders/` - Copy entire folder with your executable

### Additional (if using video):
- FFmpeg DLLs (avcodec, avformat, avutil, swscale)

## 🔧 System Requirements

**OS**: Windows 10/11 (64-bit)

**GPU**: DirectX 12 or Vulkan compatible
- NVIDIA GTX 1000 series or newer
- AMD RX 5000 series or newer
- Intel Arc or Iris Xe

**Runtime**:
- Visual C++ 2022 Redistributable (x64)
- DirectX 12
- Vulkan Runtime (optional)

## 📝 API Highlights

### Core Classes

- **CompositionEngine** - Main rendering engine
- **VideoLayer** - Video/panorama layer
- **ImageLayer** - Image/texture layer
- **ShaderEffect** - Shader-based effects
- **VideoExporter** - Export to video files
- **InputSystem** - Event-driven input handling

### Quick Export Example

```cpp
VideoExporter exporter;
VideoExportConfig config = {
    .outputPath = "output.mp4",
    .width = 1920,
    .height = 1080,
    .fps = 30,
    .bitrate = 20000000
};

bool success = exporter.exportVideoSimple(
    &engine,
    config,
    duration,
    [&]() { updateFrame(); },
    [&]() { return !shouldStop; }
);
```

## 🐛 Troubleshooting

### "DLL not found" Error
- Ensure all DLLs from `bin/` are copied to your executable directory
- Check that shaders folder is in the same directory as your .exe

### "Cannot find package ClipEngine"
- Verify `CMAKE_PREFIX_PATH` points to the SDK root folder
- Ensure `lib/cmake/ClipEngine/ClipEngineConfig.cmake` exists

### Black Screen / No Render
- Check that shaders folder is properly copied
- Verify GPU supports DirectX 12 or Vulkan
- Check device initialization: `engine.getDevice()` should not be null

### Video Export Issues
- Ensure FFmpeg DLLs are in the executable directory
- Check sufficient disk space
- Verify video duration and FPS are valid

## 📧 Support

For issues or questions about the SDK, please check:
- Sample application for reference implementation
- This distribution guide for setup instructions
- Header comments in `include/clipengine/` for API documentation

## 📄 License

ClipEngine SDK - Internal Distribution
Copyright © 2025 All rights reserved.

---

**Version**: 1.0.0
**Build Date**: 2025-11-07
**Target Platform**: Windows x64
