#pragma once

#include <clipengine/core/CompositionEngine.h>
#include <clipengine/layers/VideoRenderer.h>
#ifdef CLIPENGINE_DEBUG_WINDOW_ENABLED
#include <clipengine/debug/DebugWindow.h>
#endif
#include <clipengine/export/VideoExporter.h>
#include <clipengine/effects/ShaderEffect.h>
#include <clipengine/input/InputSystem.h>
#include <clipengine/input/InputSystemGLFWAdapter.h>

#include <GLFW/glfw3.h>
#include <string>
#include <memory>

#include "VideoSource.h"

class Application
{
public:
    void initialize();

    void run();

    bool isInitialized() const { return initialized_; }

private:
    void setupScene();
    void setupInputCallbacks();
    void setupInputEventListeners();  // New: Setup InputSystem event listeners
    void exportVideo();
    void switchRenderMode(VideoRenderer::RenderMode mode, float yaw, float pitch, float zoom);
    void updateVideoFrame();

    // Input event handlers (InputSystem)
    void handlePointerDown(const clipengine::InputEvent& event);
    void handlePointerMove(const clipengine::InputEvent& event);
    void handlePointerUp(const clipengine::InputEvent& event);
    void handleScroll(const clipengine::InputEvent& event);
    void handleKeyDown(const clipengine::InputEvent& event);

    GLFWwindow* window_ = nullptr;
    uint32_t    width_ = 1920;
    uint32_t    height_ = 1080;
    std::string title_ = "ClipEngine Video Example";

    CompositionEngine engine_;
    VideoRenderer* videoRenderer_ = nullptr;
#ifdef CLIPENGINE_DEBUG_WINDOW_ENABLED
    DebugWindow debugWindow_;
#endif

    // InputSystem for event-driven input handling
    clipengine::InputSystem inputSystem_;
    std::unique_ptr<clipengine::InputSystemGLFWAdapter> inputAdapter_;

    // Video source management (encapsulates decoder and frame handling)
    std::unique_ptr<VideoSource> videoSource_;

    // Interaction state
    bool dragging_ = false;
    double lastMouseX_ = 0.0, lastMouseY_ = 0.0;
    float yaw_ = 0.0f, pitch_ = 0.0f, zoom_ = 1.0f;

    // Export state
    bool isExporting_ = false;

    // Color adjustment filter
    ShaderEffect* colorAdjust_ = nullptr;
    float brightness_ = 0.0f;
    float contrast_ = 1.0f;
    float saturation_ = 1.0f;
    float exposure_ = 0.0f;
    float gain_ = 1.0f;

    // Initialization state
    bool initialized_ = false;
};