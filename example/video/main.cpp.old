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

#include "FrameRenderer.h"

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

    FrameRenderer texture_renderer(engine_.context_.device_, engine_.context_.surface_texture_fmt_);
    std::unique_ptr<FrameRenderer> frameRenderer = std::make_unique<FrameRenderer>(texture_renderer);
    engine_.addRenderer(std::move(frameRenderer));

    Decoder decoder;
    decoder.open_video("D:/video/8K.mp4", [&](AVFrame* frame) {
        if(frame->format == AV_PIX_FMT_D3D11) {
            ID3D11Texture2D* srcTex = (ID3D11Texture2D*)frame->data[0];
            int subIndex = (int)frame->data[1];

            dynamic_cast<FrameRenderer*>(engine_.renderers_[0].get())->update_texture(srcTex, subIndex);

            // wgpu::TextureViewDescriptor uvViewDesc = {
            //     .format = wgpu::TextureFormat::RG8Unorm,
            //     .dimension = wgpu::TextureViewDimension::e2D,
            // };
            // wgpu::TextureView uvView = sharedTexture.CreateView(&uvViewDesc);
            engine_.render();

        }

        
    });
    while(!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
    }
    return 0;
}