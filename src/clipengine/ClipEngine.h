#pragma once

// ClipEngine - Professional Video Editing Engine
// Main entry point header

// Core modules
// #include "core/RHIHelper.h"  // Optional helper, not required by default

// Render modules
#include "render/Renderer.h"
#include "render/RenderEngine.h"
#include "render/VideoRenderer.h"

// Codec modules (placeholder)
// #include "codec/Decoder.h"
// #include "codec/Encoder.h"
// #include "codec/CodecFactory.h"

// Shader modules
// Shaders are loaded at runtime from clipengine/shader/

namespace ClipEngine {

// Engine version
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;

inline const char* GetVersion() {
    return "ClipEngine v1.0.0";
}

} // namespace ClipEngine
