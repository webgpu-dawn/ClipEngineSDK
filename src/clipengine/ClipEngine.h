#pragma once

// ClipEngine - Professional Video Editing Engine
// Main entry point header

// Core modules
// #include "core/RHIHelper.h"  // Optional helper, not required by default

// Render modules
#include "render/Renderer.h"
#include "render/RenderEngine.h"
#include "render/VideoRenderer.h"

// Utility modules
#include "util/Logger.h"

// Shader modules
// Shaders are loaded at runtime from clipengine/shader/

namespace ClipEngine {

const char* getVersion();

} // namespace ClipEngine
