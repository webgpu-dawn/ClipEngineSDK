#pragma once

#include <clipengine/core/CompositionEngine.h>
// 新架构 - Layer + Effect API
#include <clipengine/layers/VideoLayer.h>
#include <clipengine/layers/ImageLayer.h>
#include <clipengine/effects/ColorAdjustEffect.h>
// 旧架构 - 临时保留用于渲染
#include <clipengine/layers/VideoRenderer.h>
#include <clipengine/layers/TextureRenderer.h>
#ifdef CLIPENGINE_DEBUG_WINDOW_ENABLED
#include <clipengine/debug/DebugWindow.h>
#endif
#include <clipengine/export/VideoExporter.h>
#include <clipengine/input/InputSystem.h>
#include <clipengine/input/InputSystemGLFWAdapter.h>

#include <GLFW/glfw3.h>
#include <string>
#include <memory>

#include "VideoSource.h"
#include "ImageLoader.h"

class Application
{
public:
    void initialize();

    void run();

    bool isInitialized() const { return initialized_; }

private:
    void setupScene();
    void setupInputCallbacks();
    void setupInputEventListeners();
    void exportVideo();
    void switchRenderMode(VideoRenderer::RenderMode mode, float yaw, float pitch, float zoom);
    void updateVideoFrame();
    void loadImageTexture(const std::string& imagePath);

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

    // 新架构 - Layer + Effect API
    std::shared_ptr<clipengine::VideoLayer> videoLayer_;
    std::shared_ptr<clipengine::ImageLayer> imageLayer_;
    std::shared_ptr<clipengine::ColorAdjustEffect> colorEffect_;

    // 旧架构 - 临时保留的渲染器指针（用于实际渲染）
    VideoRenderer* videoRenderer_ = nullptr;
    TextureRenderer* imageRenderer_ = nullptr;

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

    // 颜色调整参数 (新 API: 1.0 = 正常)
    float brightness_ = 1.0f;  // 新 API: 0.0 - 2.0, 1.0 = 正常
    float contrast_ = 1.0f;
    float saturation_ = 1.0f;
    float exposure_ = 0.0f;
    float gain_ = 1.0f;

    // Initialization state
    bool initialized_ = false;
};