#include "Application.h"
#include "Decoder.h"

#include <iostream>

using namespace std;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void Application::initialize() 
{
    // 初始化窗口
    width_ = 640;
    height_= 480;
    title_ = "ClipEngine - Example";
    if(!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
    if(!window_) {
        std::cerr << "Could not open window" << std::endl;
        glfwTerminate();
        return;
    }

    // 初始化 ce
    CeConfigure config = {
        .width  = width_,
        .height = height_,
        .window_title = title_
    };
    if(!ce_.initialize(config)) {
        std::cerr << "Failed to initialize clip engine" << std::endl;
        return;
    }

    auto renderer = std::make_unique<VideoRenderer>();
    renderer->setName("test");
    renderer->setViewport(0, 0, 1, 1);
    ce_.addRenderer(std::move(renderer));
}

void Application::run()
{
    Decoder decoder;
    decoder.open_video("D:/video/8K.mp4", [&](AVFrame* frame) {
        if(frame->format == AV_PIX_FMT_D3D11) {
            ID3D11Texture2D* srcTex = (ID3D11Texture2D*)frame->data[0];
            int subIndex = (int)(intptr_t)frame->data[1];

            CeRenderable* renderer = ce_.getRendererByName("test");
            if(renderer) {
                ((VideoRenderer*)renderer)->updateFrame(srcTex, subIndex);
            }
            
            ce_.renderFrame();
        }
    });

    while(!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
    }
}