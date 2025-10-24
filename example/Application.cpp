#include "Application.h"
#include "Decoder.h"

#include <iostream>
#include <chrono>
#include <thread>

#include <clipengine/effects/ShaderEffect.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

using namespace std;

void Application::initialize()
{
    // Initialize GLFW window
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

    // Initialize CompositionEngine (manages RenderDevice internally)
    DeviceConfig config = {
        .width = width_,
        .height = height_,
        .window_title = title_,
        .hwnd = glfwGetWin32Window(window_)
    };

    if (!engine_.initialize(config)) {
        std::cerr << "Failed to initialize CompositionEngine" << std::endl;
        return;
    }

    setupScene();
    setupInputCallbacks();

    // Initialize Debug Window (standalone window)
    if (!debugWindow_.initialize(&engine_, "ClipEngine Debug")) {
        std::cerr << "Failed to initialize Debug Window" << std::endl;
        return;
    }

    // Attach debug window to engine for automatic updates
    engine_.setDebugWindow(&debugWindow_);

    std::cout << "Application initialized successfully" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  - Left click drag to rotate panorama view" << std::endl;
    std::cout << "  - Mouse wheel to zoom panorama" << std::endl;
    std::cout << "  - Debug window opened in separate window" << std::endl;
    std::cout << "  - ESC to exit" << std::endl;
}

void Application::setupScene()
{
    // Create video renderer for panorama mode
    auto video = std::make_unique<VideoRenderer>();
    video->setTransform(0.0f, 0.0f, 1.0f, 1.0f);
    video->setLayer(0);
    video->setName("Main Video Layer");
    video->setAspect((float)width_ / (float)height_);
    size_t videoIdx = engine_.addLayer(std::move(video));

    videoRenderer_ = static_cast<VideoRenderer*>(engine_.getLayer(videoIdx));

    // Set panorama mode after layer initialization
    if (videoRenderer_) {
        videoRenderer_->setRenderMode(VideoRenderer::RenderMode::Panorama);
    }

    // Load vintage film effect from configuration file
    // Default parameter values are automatically applied from the config file
    auto vintageFilm = ShaderEffect::loadFromFile("effects/vintage_film.effect");
    if (vintageFilm) {
        // Parameters are already set to defaults from config file
        // You can still override them if needed:
        // vintageFilm->setParam("sepiaIntensity", 0.9f);
        colorAdjustEffect_ = vintageFilm.get();
        engine_.getGlobalFilterChain().addFilter(std::move(vintageFilm));
    } else {
        std::cerr << "Failed to load vintage film effect, falling back to color adjust" << std::endl;
        // Fallback to built-in effect
        auto colorAdjust = ShaderEffect::createColorAdjust();
        colorAdjustEffect_ = colorAdjust.get();
        engine_.getGlobalFilterChain().addFilter(std::move(colorAdjust));
    }
}

void Application::setupInputCallbacks()
{
    glfwSetWindowUserPointer(window_, this);
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

            {
                std::lock_guard<std::mutex> lock(frameMutex_);
                frameData_.texture = srcTex;
                frameData_.subIndex = subIndex;
                frameData_.hasNewFrame = true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    });

    while(!glfwWindowShouldClose(window_)) {
        // Update video frame (thread-safe) - minimize lock scope
        ID3D11Texture2D* currentTexture = nullptr;
        int currentSubIndex = 0;
        bool hasFrame = false;

        {
            std::lock_guard<std::mutex> lock(frameMutex_);
            if (frameData_.hasNewFrame) {
                currentTexture = frameData_.texture;
                currentSubIndex = frameData_.subIndex;
                hasFrame = true;
                frameData_.hasNewFrame = false;
            }
        }

        // Perform GPU operations outside the lock to avoid blocking decoder thread
        if (hasFrame && videoRenderer_) {
            videoRenderer_->updateFrame(currentTexture, currentSubIndex);
        }

        glfwPollEvents();

        // Handle panorama rotation (mouse drag)
        if (videoRenderer_) {
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

            // Mouse wheel zoom control
            const InputState& input = engine_.getInputState();
            if (input.mouse.wheelDelta != 0.0f) {
                zoom_ += input.mouse.wheelDelta * 0.1f;
                if (zoom_ < 0.1f) zoom_ = 0.1f;
                if (zoom_ > 5.0f) zoom_ = 5.0f;
                videoRenderer_->setZoom(zoom_);
            }
        }

        // Render frame (all-in-one: updates, renders, presents)
        engine_.render();
    }

    glfwTerminate();
}

void Application::cursorPosCallback(GLFWwindow* window, double x, double y)
{
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->engine_.setMousePosition(static_cast<float>(x), static_cast<float>(y));
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->engine_.setMouseButton(button, action == GLFW_PRESS);
}

void Application::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->engine_.setMouseWheel(static_cast<float>(yoffset));
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->engine_.setKeyState(key, action == GLFW_PRESS || action == GLFW_REPEAT);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}
