#pragma once

#include <webgpu/webgpu.h>
#include <GLFW/glfw3.h>

class Application {

public:
    bool initialize();
    void terminate();
    void run();

private:
    GLFWwindow* window_;

    WGPUInstance instance_;
    WGPUDevice   device_;
    WGPUQueue    queue_;
};