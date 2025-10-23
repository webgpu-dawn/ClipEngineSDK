#pragma once

#include <clipengine/clipengine.h>

// forward-declare to avoid requiring the render header in the public app header
class PanoramaRenderer;

#include <GLFW/glfw3.h>
#include <string>

class Application
{
public:
    void initialize();

    void run();

private:
    GLFWwindow* window_;
    uint32_t    width_;
    uint32_t    height_;
    std::string title_;

    ClipEngine  ce_;
    PanoramaRenderer* panoramaRenderer_ = nullptr;

    // interaction state
    bool dragging_ = false;
    double lastMouseX_ = 0.0, lastMouseY_ = 0.0;
    float yaw_ = 0.0f, pitch_ = 0.0f, zoom_ = 1.0f;

};