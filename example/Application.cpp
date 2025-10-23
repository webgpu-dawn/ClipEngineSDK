#include "Application.h"
#include "Decoder.h"

#include <iostream>

using namespace std;

#include <clipengine/render/PanoramaRenderer.h>
#include <clipengine/render/VideoRenderer.h>
#include <clipengine/render/TextureRenderer.h>

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

    // Panorama renderer (disabled by default) - will sample the same video texture
    auto pano = std::make_unique<PanoramaRenderer>();
    pano->setName("panorama");
    pano->setViewport(0, 0, 1, 1);
    pano->setLayer(0);
    pano->setEnabled(false);
    panoramaRenderer_ = pano.get();
    // set aspect ratio for correct perspective mapping
    panoramaRenderer_->setAspect((float)width_ / (float)height_);
    ce_.addRenderer(std::move(pano));

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
    decoder.open_video("D:/video/Q360_19700103_032841_000001_Output(11).mp4", [&](AVFrame* frame) {
        if(frame->format == AV_PIX_FMT_D3D11) {
            ID3D11Texture2D* srcTex = (ID3D11Texture2D*)frame->data[0];
            int subIndex = (int)(intptr_t)frame->data[1];

            // 更新主视频 renderer（保持主视频渲染器更新）
            CeRenderable* mainRenderer = ce_.getRendererByName("main_video");
            if (mainRenderer) {
                ((VideoRenderer*)mainRenderer)->updateFrame(srcTex, subIndex);
            }

            // 将主渲染器的 texture views 转发给 panorama renderer
            CeRenderable* panoR = ce_.getRendererByName("panorama");
            if (mainRenderer && panoR) {
                TextureRenderer* texMain = static_cast<TextureRenderer*>(mainRenderer);
                const auto& views = texMain->getTextureViews();
                if (!views.empty()) {
                    // 启用 panorama 并把 views 传递过去
                    panoR->setEnabled(true);
                    PanoramaRenderer* pano = static_cast<PanoramaRenderer*>(panoR);
                    pano->applyTextureViews(views);
                }
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
        // Poll events and handle input
        glfwPollEvents();

        // Mouse handling: left drag to rotate, scroll to zoom
        if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            double mx, my;
            glfwGetCursorPos(window_, &mx, &my);
            if (!dragging_) {
                dragging_ = true;
            } else {
                double dx = mx - lastMouseX_;
                double dy = my - lastMouseY_;
                // sensitivity
                yaw_ += (float)(dx * 0.005);
                pitch_ += (float)(dy * 0.005);
                if (pitch_ > 1.5f) pitch_ = 1.5f;
                if (pitch_ < -1.5f) pitch_ = -1.5f;
                if (panoramaRenderer_) panoramaRenderer_->setRotation(yaw_, pitch_);
            }
            lastMouseX_ = mx; lastMouseY_ = my;
        } else {
            dragging_ = false;
        }

        // scroll callback via polling (GLFW doesn't provide polling scroll, so use a callback - simplified here)
        // For brevity, we use glfwGetKey for +/- to control zoom
        if (glfwGetKey(window_, GLFW_KEY_KP_ADD) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_EQUAL) == GLFW_PRESS) {
            zoom_ *= 1.01f;
            if (panoramaRenderer_) panoramaRenderer_->setZoom(zoom_);
        }
        if (glfwGetKey(window_, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_MINUS) == GLFW_PRESS) {
            zoom_ *= 0.99f;
            if (panoramaRenderer_) panoramaRenderer_->setZoom(zoom_);
        }
    }
}