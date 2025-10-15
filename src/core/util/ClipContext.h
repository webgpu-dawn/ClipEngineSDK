#pragma
#define GLFW_EXPOSE_NATIVE_WIN32
#include <webgpu/webgpu_cpp.h>

#include <vector>

#include "ComboRenderPipelineDescriptor.h"
#include "WGPUHelpers.h"

struct GLFWwindow;
class ClipEngine;

/**
 * 负责创建设备，队列，surface
 */
class ClipContext {

public:
    bool initialize(GLFWwindow* window);

    void begin();
    void end();

private:
    std::unique_ptr<wgpu::ChainedStruct> setup_window_and_get_surface_desciptor(GLFWwindow* window);
    wgpu::Surface create_surface_for_window(const wgpu::Instance& instance, GLFWwindow* window);

    friend class ClipEngine;
public:
    wgpu::Instance       instance_;
    wgpu::Adapter        adapter_;
    wgpu::Device         device_;
    wgpu::Queue          queue_;
    wgpu::Surface        surface_;
    wgpu::RenderPipeline pipeline_;

    wgpu::TextureFormat surface_texture_fmt_ = wgpu::TextureFormat::BGRA8Unorm;


};