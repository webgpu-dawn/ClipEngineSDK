#pragma once

#include <clipengine/core/CompositionEngine.h>
#include <clipengine/layers/VideoRenderer.h>

#include <GLFW/glfw3.h>
#include <string>
#include <mutex>

struct ID3D11Texture2D;

class Application
{
public:
    void initialize();

    void run();

private:
    void setupScene();
    void setupInputCallbacks();

    // GLFW input callbacks
    static void cursorPosCallback(GLFWwindow* window, double x, double y);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* window_ = nullptr;
    uint32_t    width_ = 1920;
    uint32_t    height_ = 1080;
    std::string title_ = "ClipEngine Video Example";

    CompositionEngine engine_;  // CompositionEngine now manages CeContext internally
    VideoRenderer* videoRenderer_ = nullptr;

    // 视频帧数据（线程安全传递）
    struct FrameData {
        ID3D11Texture2D* texture = nullptr;
        int subIndex = 0;
        bool hasNewFrame = false;
    };
    FrameData frameData_;
    std::mutex frameMutex_;

    // 交互状态
    bool dragging_ = false;
    double lastMouseX_ = 0.0, lastMouseY_ = 0.0;
    float yaw_ = 0.0f, pitch_ = 0.0f, zoom_ = 1.0f;
};