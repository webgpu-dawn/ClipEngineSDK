# ClipEngine Debug Module

## Overview

The Debug module provides a standalone debug window for inspecting and controlling ClipEngine's internal state in real-time.

**Features:**
- 🪟 **Standalone Window** - Runs in a separate window, doesn't obstruct main render view
- 🔍 **Engine Inspection** - View engine configuration, layers, and filters
- 🎨 **Live Parameter Editing** - Adjust shader parameters in real-time
- 📊 **Layer Management** - Enable/disable layers, view properties
- 🎛️ **Color Adjustment** - Quick color grading controls

## Architecture

**DebugWindow** is designed as an optional, source-included component:
- Source files are provided in SDK but **not compiled into the main library**
- Applications can choose to include and compile it with their own ImGui/GLFW dependencies
- This keeps the core SDK lightweight and dependency-free

## Usage

### 1. Include in Your Project

In your CMakeLists.txt:

```cmake
add_executable(your_app
    main.cpp
    # ... other sources ...

    # Include DebugWindow from SDK
    ${ClipEngine_INCLUDE_DIRS}/../../../src/clipengine/debug/DebugWindow.cpp
)

# Make sure you have ImGui and GLFW
target_link_libraries(your_app
    ClipEngine::clipengine
    imgui_core
    imgui_backends
    glfw
)
```

### 2. Initialize Debug Window

```cpp
#include <clipengine/debug/DebugWindow.h>

// Create your engine
CompositionEngine engine;
engine.initialize(...);

// Create debug window
DebugWindow debugWindow;
if (!debugWindow.initialize(&engine, "ClipEngine Debug")) {
    std::cerr << "Failed to initialize debug window" << std::endl;
}
```

### 3. Update in Main Loop

```cpp
while (!shouldClose && !debugWindow.shouldClose()) {
    // Update main application
    engine.update(deltaTime);
    engine.render(outputView);

    // Update debug window (non-blocking)
    debugWindow.update();
}

debugWindow.shutdown();
```

## Dependencies

DebugWindow requires:
- **ImGui** (docking branch recommended)
- **GLFW** 3.x
- **WebGPU/Dawn** (already provided by ClipEngine)

## Features

### 1. Overview Tab
- Engine configuration (format, backend)
- Layer count and statistics
- Global filter count

### 2. Layers Tab
- Hierarchical layer view
- Enable/disable individual layers
- Layer properties:
  - Name, type, z-order
  - Transform (position, size)
  - Render mode (for video layers)
- Per-layer filter chains

### 3. Global Filters Tab
- View all global filters
- Shader parameter inspection
- Real-time parameter adjustment

### 4. Color Adjust Tab
- Quick color grading controls
- Automatically finds ColorAdjust effects
- Common adjustments:
  - Brightness, Contrast, Saturation
  - Exposure, Gamma
  - Temperature, Tint

## API Reference

### DebugWindow Class

```cpp
class DebugWindow {
public:
    // Initialize with engine reference
    bool initialize(CompositionEngine* engine,
                   const char* title = "ClipEngine Debug",
                   int width = 600,
                   int height = 800);

    // Shutdown and cleanup
    void shutdown();

    // Update debug window (call once per frame)
    void update();

    // Check if window should close
    bool shouldClose() const;

    // Window visibility
    void setVisible(bool visible);
    bool isVisible() const;
};
```

## Example

See `example/Application.cpp` for a complete integration example.

## Notes

- Debug window shares the WebGPU device with the main engine
- Window can be moved to a second monitor for dual-screen debugging
- Closing either window (main or debug) will exit the application
- All parameters are editable and take effect immediately

## Future Enhancements

Potential additions:
- Performance metrics (FPS, frame time, GPU time)
- Texture preview
- Memory usage monitoring
- Timeline/playback controls
- Custom debug overlays
