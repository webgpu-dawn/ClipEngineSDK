# ClipEngine Shaders

This directory contains WGSL (WebGPU Shading Language) shaders used by ClipEngine.

## Available Shaders

### video.wgsl
Video rendering shader that performs NV12 to RGB conversion using BT.709 color space.
- **Inputs**: Y plane (R8Unorm) and UV plane (RG8Unorm)
- **Output**: RGBA8 surface
- **Features**: BT.709 color space conversion, linear sampling

## Future Shaders (Planned)

- **effect.wgsl** - Post-processing effects (blur, sharpen, color grading)
- **transition.wgsl** - Transition effects (fade, dissolve, wipe)
- **composite.wgsl** - Alpha blending and compositing operations

## Usage

Shaders can be loaded at runtime using the RHI Helper:

```cpp
#include "clipengine/core/RHIHelper.h"

RHI::Helper rhi(device);
auto shader = rhi.createShaderFromFile("clipengine/shader/video.wgsl");
```

Or embedded directly in code using the inline shader source from VideoRenderer.
