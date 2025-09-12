#pragma once

#include <webgpu/webgpu.h>
#include <GLFW/glfw3.h>

class Application {

public:
    bool initialize();
    void terminate();
    void run();

private:
    WGPUTextureView get_next_surface_textureview();

private:
    GLFWwindow* window_;

    WGPUDevice   device_;
    WGPUQueue    queue_;
    WGPUSurface  surface_;
};