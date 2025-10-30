#pragma once

#include <clipengine/core/CompositionEngine.h>
#include <clipengine/layers/VideoRenderer.h>
#include <clipengine/debug/DebugWindow.h>
#include <clipengine/export/VideoExporter.h>
#include <clipengine/effects/ShaderEffect.h>

#include <GLFW/glfw3.h>
#include <string>
#include <mutex>
#include <memory>

struct ID3D11Texture2D;
class Decoder;

class Application
{
public:
    void initialize();

    void run();

private:
    void setupScene();
    void setupInputCallbacks();
    void exportVideo();
    void switchRenderMode(VideoRenderer::RenderMode mode, float yaw, float pitch, float zoom);

    bool updateVideoFrame();
    void setupExportDecoder();
    void waitForFrames(int milliseconds);

    // GLFW input callbacks
    static void cursorPosCallback(GLFWwindow* window, double x, double y);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* window_ = nullptr;
    uint32_t    width_ = 1920;
    uint32_t    height_ = 1080;
    std::string title_ = "ClipEngine Video Example";

    CompositionEngine engine_;
    VideoRenderer* videoRenderer_ = nullptr;

    DebugWindow debugWindow_;

    // Video frame data (thread-safe transfer)
    struct FrameData {
        ID3D11Texture2D* texture = nullptr;
        int subIndex = 0;
        bool hasNewFrame = false;
    };
    FrameData frameData_;
    std::mutex frameMutex_;
    std::atomic<bool> allowPlaybackDecoderUpdates_{true};  // Control playback decoder

    // Interaction state
    bool dragging_ = false;
    double lastMouseX_ = 0.0, lastMouseY_ = 0.0;
    float yaw_ = 0.0f, pitch_ = 0.0f, zoom_ = 1.0f;

    // Export state
    bool isExporting_ = false;
    bool allowExportDecoderUpdates_ = false;
    std::shared_ptr<Decoder> exportDecoder_;

    // Color adjustment filter
    ShaderEffect* colorAdjust_ = nullptr;
    float brightness_ = 0.0f;
    float contrast_ = 1.0f;
    float saturation_ = 1.0f;
    float exposure_ = 0.0f;
    float gain_ = 1.0f;
};