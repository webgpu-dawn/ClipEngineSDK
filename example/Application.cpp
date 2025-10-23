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
    width_ = 1920;
    height_= 1080;
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

    // 示例1: 使用默认的 NV12 video renderer (全屏)
    auto mainVideoRenderer = std::make_unique<VideoRenderer>();
    mainVideoRenderer->setName("main_video");
    mainVideoRenderer->setViewport(0, 0, 1, 1);
    mainVideoRenderer->setLayer(0);
    ce_.addRenderer(std::move(mainVideoRenderer));

    // 示例2: 使用自定义 shader 的 TextureRenderer (右上角小窗口)
    // 这展示了如何创建一个使用自定义 shader 的 renderer
    auto customShaderConfig = ShaderPresets::createColorShader();
    auto customRenderer = std::make_unique<TextureRenderer>(customShaderConfig);
    customRenderer->setName("custom_overlay");
    customRenderer->setViewport(0.7f, 0.0f, 0.3f, 0.3f);  // 右上角 30%x30%
    customRenderer->setLayer(1);  // 在主视频之上
    customRenderer->setEnabled(false);  // 暂时禁用，可以动态启用
    ce_.addRenderer(std::move(customRenderer));

    // 示例3: 可以添加更多 renderer，比如字幕、特效等
    // auto subtitleRenderer = std::make_unique<TextureRenderer>(ShaderPresets::createRGBATextureShader());
    // subtitleRenderer->setName("subtitle");
    // subtitleRenderer->setViewport(0, 0.8f, 1, 0.2f);  // 底部字幕区域
    // subtitleRenderer->setLayer(2);
    // ce_.addRenderer(std::move(subtitleRenderer));
}

void Application::run()
{
    Decoder decoder;
    decoder.open_video("D:/video/8K.mp4", [&](AVFrame* frame) {
        if(frame->format == AV_PIX_FMT_D3D11) {
            ID3D11Texture2D* srcTex = (ID3D11Texture2D*)frame->data[0];
            int subIndex = (int)(intptr_t)frame->data[1];

            // 更新主视频 renderer
            CeRenderable* mainRenderer = ce_.getRendererByName("main_video");
            if(mainRenderer) {
                ((VideoRenderer*)mainRenderer)->updateFrame(srcTex, subIndex);
            }

            // 可以在运行时启用/禁用其他 renderer
            // CeRenderable* customRenderer = ce_.getRendererByName("custom_overlay");
            // if (customRenderer) {
            //     customRenderer->setEnabled(true);  // 动态启用叠加层
            // }

            ce_.renderFrame();
        }
    });

    while(!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
    }
}