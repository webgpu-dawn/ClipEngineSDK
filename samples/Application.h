#pragma once

#include <clipengine/core/CompositionEngine.h>
// Layer + Effect API
#include <clipengine/layers/VideoLayer.h>
#include <clipengine/layers/ImageLayer.h>
#include <clipengine/effects/ColorAdjustEffect.h>
#include <clipengine/effects/ShaderEffect.h>
#include <clipengine/export/VideoExporter.h>
#include <clipengine/input/InputSystem.h>
#include <clipengine/input/InputSystemGLFWAdapter.h>

#include <GLFW/glfw3.h>
#include <string>
#include <memory>

#include "VideoSource.h"
#include "PanoramaController.h"

class Application
{
public:
    void initialize();

    void run();

    bool isInitialized() const { return initialized_; }

    // Test helper: trigger export programmatically
    void testExport() { exportVideo(); }

private:
    void setupScene();
    void setupInputCallbacks();
    void setupInputEventListeners();
    void exportVideo();
    void updateVideoFrame();
    void loadImageTexture(const std::string& imagePath);

    // Input event handlers (InputSystem)
    void handleKeyDown(const clipengine::InputEvent& event);

    GLFWwindow* window_ = nullptr;
    uint32_t    width_ = 1920;
    uint32_t    height_ = 1080;
    std::string title_ = "ClipEngine Video Example";

    CompositionEngine engine_;

    // Layer + Effect API
    VideoLayer* videoLayer_ = nullptr;      // Main video layer
    ImageLayer* imageLayer_ = nullptr;      // Image overlay layer
    ShaderEffect* colorEffect_ = nullptr;   // Color adjustment effect

    // InputSystem for event-driven input handling
    clipengine::InputSystem inputSystem_;
    std::unique_ptr<clipengine::InputSystemGLFWAdapter> inputAdapter_;

    // Video source management (encapsulates decoder and frame handling)
    std::unique_ptr<VideoSource> videoSource_;

    // Panorama interaction controller
    PanoramaController panoramaController_;

    // Export state
    bool isExporting_ = false;
    bool shouldExport_ = false;  // Flag to trigger export outside main loop

    // Initialization state
    bool initialized_ = false;
};