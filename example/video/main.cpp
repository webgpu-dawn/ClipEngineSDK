#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <webgpu/webgpu_cpp.h>

#include <iostream>
#include <chrono>
#include <thread>
#include "decoder.h"
#include <filesystem>

#include "../src/core/ClipEngine.h"
#include "../src/core/util/ComboRenderPipelineDescriptor.h"
#include "../src/media_render/core/RenderEngine.h"
#include "../src/media_render/renderers/VideoRenderer.h"

using namespace std;
namespace fs = std::filesystem;

GLFWwindow* window_;
ClipEngine engine_;

int w = 400;
int h = 400;

int main()
{
    fs::path exe_dir = fs::current_path();
    fs::current_path(exe_dir);

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(w, h, "Application", nullptr, nullptr);

    engine_.initialize(window_);

    auto videoRenderer = std::make_unique<MediaRender::VideoRenderer>();
    videoRenderer->initialize(engine_.context_.device_, engine_.context_.surface_texture_fmt_);
    videoRenderer->setViewport(0.0f, 0.0f, 0.5f, 1.0f);

    MediaRender::VideoRenderer* videoRendererPtr = videoRenderer.get();
    MediaRender::RenderEngine renderEngine;
    renderEngine.initialize(engine_.context_.device_, engine_.context_.surface_texture_fmt_);
    renderEngine.addRenderer(std::move(videoRenderer));

    Decoder decoder;
    decoder.open_video("D:/video/video.mp4", [&](AVFrame* frame) {
        if(frame->format == AV_PIX_FMT_D3D11) {
            ID3D11Texture2D* srcTex = (ID3D11Texture2D*)frame->data[0];
            int subIndex = (int)frame->data[1];

            videoRendererPtr->updateFrame(srcTex, subIndex);

            wgpu::SurfaceTexture surface_texture;
            engine_.context_.surface_.GetCurrentTexture(&surface_texture);
            dawn::utils::ComboRenderPassDescriptor render_pass({ surface_texture.texture.CreateView()});

            wgpu::CommandEncoder encoder = engine_.context_.device_.CreateCommandEncoder();
            {
                wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&render_pass);
                renderEngine.render(pass);
                pass.End();
            }

            wgpu::CommandBuffer commands = encoder.Finish();
            engine_.context_.queue_.Submit(1, &commands);
            engine_.context_.surface_.Present();
        }
    });
    while(!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
    }
    return 0;
}