#pragma once

#include <webgpu/webgpu.hpp>

struct GLFWwindow;

class Application {

public:
    bool initialize();
    void terminate();
    void run();

private:
    wgpu::TextureView get_next_surface_view();

private:
    GLFWwindow* window_ = nullptr;

    wgpu::Instance instance_ = nullptr;
    wgpu::Device   device_   = nullptr;
    wgpu::Queue    queue_    = nullptr;
    wgpu::Surface  surface_  = nullptr;
};