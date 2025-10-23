#include "Application.h"
#include "Decoder.h"

#include <iostream>
#include <chrono>
#include <thread>

#include <clipengine/render/ShaderEffect.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

using namespace std;

void Application::initialize()
{
    // 初始化 GLFW 窗口
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

    // 初始化 WebGPU 上下文
    CeConfigure config = {
        .width = width_,
        .height = height_,
        .window_title = title_,
        .hwnd = glfwGetWin32Window(window_)
    };

    if (!context_.initialize(config)) {
        std::cerr << "Failed to initialize WebGPU context" << std::endl;
        return;
    }

    // 初始化 VideoRenderEngine
    engine_.initialize(context_.getDevice(), context_.getSurfaceFormat(), width_, height_);

    // 设置场景（添加渲染器和特效）
    setupScene();

    // 设置 GLFW 输入回调
    setupInputCallbacks();

    std::cout << "Application initialized successfully" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  - Move mouse to see spotlight effect" << std::endl;
    std::cout << "  - Press P to toggle panorama view" << std::endl;
    std::cout << "  - Left click drag to rotate panorama view" << std::endl;
    std::cout << "  - Mouse wheel to zoom panorama" << std::endl;
    std::cout << "  - +/- keys to adjust zoom" << std::endl;
    std::cout << "  - ESC to exit" << std::endl;
}

void Application::setupScene()
{
    // 创建统一的视频渲染器（全屏）
    auto video = std::make_unique<VideoRenderer>();
    video->setViewport(0.0f, 0.0f, 1.0f, 1.0f);
    video->setLayer(0);
    video->setAspect((float)width_ / (float)height_);  // 设置宽高比用于全景模式
    size_t videoIdx = engine_.addRenderable(std::move(video));

    // 保存视频渲染器指针用于后续更新
    videoRenderer_ = static_cast<VideoRenderer*>(engine_.getRenderable(videoIdx));

    // 添加全局交互特效
    // 1. 鼠标聚光灯效果（跟随鼠标）
    auto spotlight = ShaderEffect::createMouseSpotlight();
    spotlight->setParam("radius", 0.25f);      // 聚光灯半径
    spotlight->setParam("intensity", 0.6f);    // 暗部强度
    engine_.getGlobalFilterChain().addFilter(std::move(spotlight));

    // 2. 色彩调整（可选，默认禁用）
    // auto colorAdjust = ShaderEffect::createColorAdjust();
    // colorAdjust->setParam("brightness", 0.0f);
    // colorAdjust->setParam("contrast", 1.0f);
    // colorAdjust->setParam("saturation", 1.0f);
    // engine_.getGlobalFilterChain().addFilter(std::move(colorAdjust));
}

void Application::setupInputCallbacks()
{
    // 设置窗口用户指针，用于在静态回调中访问 Application 实例
    glfwSetWindowUserPointer(window_, this);

    // 设置 GLFW 输入回调
    glfwSetCursorPosCallback(window_, cursorPosCallback);
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    glfwSetScrollCallback(window_, scrollCallback);
    glfwSetKeyCallback(window_, keyCallback);
}

void Application::run()
{
    Decoder decoder;
    decoder.open_video("D:/video/8K.mp4", [&](AVFrame* frame) {
        if(frame->format == AV_PIX_FMT_D3D11) {
            ID3D11Texture2D* srcTex = (ID3D11Texture2D*)frame->data[0];
            int subIndex = (int)(intptr_t)frame->data[1];

            // 在解码线程中只保存帧数据，不调用 WebGPU 函数
            {
                std::lock_guard<std::mutex> lock(frameMutex_);
                frameData_.texture = srcTex;
                frameData_.subIndex = subIndex;
                frameData_.hasNewFrame = true;
            }

            // 等待渲染线程处理完这一帧
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    });

    auto lastTime = std::chrono::high_resolution_clock::now();

    while(!glfwWindowShouldClose(window_)) {
        // 计算帧间隔时间
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // 在主渲染线程中处理新帧（线程安全）
        {
            std::lock_guard<std::mutex> lock(frameMutex_);
            if (frameData_.hasNewFrame && videoRenderer_) {
                // 更新视频帧
                videoRenderer_->updateFrame(frameData_.texture, frameData_.subIndex);
                frameData_.hasNewFrame = false;
            }
        }

        // 处理输入事件
        glfwPollEvents();

        // 手动处理 panorama 旋转（使用拖拽）- 仅在全景模式时
        if (videoRenderer_ && videoRenderer_->getRenderMode() == VideoRenderer::RenderMode::Panorama) {
            if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                double mx, my;
                glfwGetCursorPos(window_, &mx, &my);
                if (!dragging_) {
                    dragging_ = true;
                } else {
                    double dx = mx - lastMouseX_;
                    double dy = my - lastMouseY_;
                    yaw_ += (float)(dx * 0.005);
                    pitch_ += (float)(dy * 0.005);
                    if (pitch_ > 1.5f) pitch_ = 1.5f;
                    if (pitch_ < -1.5f) pitch_ = -1.5f;
                    videoRenderer_->setRotation(yaw_, pitch_);
                }
                lastMouseX_ = mx;
                lastMouseY_ = my;
            } else {
                dragging_ = false;
            }

            // 键盘控制缩放
            if (glfwGetKey(window_, GLFW_KEY_KP_ADD) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_EQUAL) == GLFW_PRESS) {
                zoom_ *= 1.01f;
                videoRenderer_->setZoom(zoom_);
            }
            if (glfwGetKey(window_, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_MINUS) == GLFW_PRESS) {
                zoom_ *= 0.99f;
                videoRenderer_->setZoom(zoom_);
            }

            // 鼠标滚轮控制缩放
            const InputState& input = engine_.getInputState();
            if (input.mouse.wheelDelta != 0.0f) {
                zoom_ += input.mouse.wheelDelta * 0.1f;
                // 限制缩放范围
                if (zoom_ < 0.1f) zoom_ = 0.1f;
                if (zoom_ > 5.0f) zoom_ = 5.0f;
                videoRenderer_->setZoom(zoom_);
            }
        }

        // 更新引擎（自动更新时间信息）
        engine_.update(deltaTime);

        // 获取当前 surface texture 并渲染
        wgpu::Surface surface = context_.getSurface();
        if (surface) {
            wgpu::SurfaceTexture surfaceTexture;
            surface.GetCurrentTexture(&surfaceTexture);

            if (surfaceTexture.texture) {
                wgpu::TextureView outputView = surfaceTexture.texture.CreateView();
                engine_.render(outputView);
                surface.Present();
            }
        }
    }

    glfwTerminate();
}

// ============================================================================
// GLFW Input Callbacks
// ============================================================================

void Application::cursorPosCallback(GLFWwindow* window, double x, double y)
{
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    // 将鼠标位置传递给引擎（引擎会自动归一化到 0-1 范围）
    app->engine_.setMousePosition(static_cast<float>(x), static_cast<float>(y));
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    // 将鼠标按钮状态传递给引擎
    app->engine_.setMouseButton(button, action == GLFW_PRESS);
}

void Application::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    // 将滚轮增量传递给引擎
    app->engine_.setMouseWheel(static_cast<float>(yoffset));
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    // 将键盘状态传递给引擎
    app->engine_.setKeyState(key, action == GLFW_PRESS || action == GLFW_REPEAT);

    // ESC 键退出
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    // P 键切换普通视图和全景视图
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        if (app->videoRenderer_) {
            auto currentMode = app->videoRenderer_->getRenderMode();
            if (currentMode == VideoRenderer::RenderMode::Planar) {
                app->videoRenderer_->setRenderMode(VideoRenderer::RenderMode::Panorama);
                std::cout << "Switched to panorama view" << std::endl;
            } else {
                app->videoRenderer_->setRenderMode(VideoRenderer::RenderMode::Planar);
                std::cout << "Switched to normal view" << std::endl;
            }
        }
    }
}
