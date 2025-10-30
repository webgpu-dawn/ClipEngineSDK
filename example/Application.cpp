#include "Application.h"
#include "Decoder.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

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

    // Setup color adjustment filter
    auto colorAdjust = ShaderEffect::createColorAdjust();

    // Set initial parameter values before adding to filter chain
    // This ensures the uniform buffer is created with correct size
    colorAdjust->setParam("brightness", 0.0f);
    colorAdjust->setParam("contrast", 1.0f);
    colorAdjust->setParam("saturation", 1.0f);
    colorAdjust->setParam("exposure", 0.0f);
    colorAdjust->setParam("gain", 1.0f);
    colorAdjust->setParam("hue", 0.0f);

    colorAdjust_ = colorAdjust.get();
    engine_.getGlobalFilterChain().addFilter(std::move(colorAdjust));

    std::cout << "ClipEngine initialized\nControls:\n"
              << "  1-4: Switch render modes | E: Export video | ESC: Exit\n"
              << "  Mouse: Drag to rotate, wheel to zoom\n"
              << "  Q/W: Brightness +/-  | A/S: Contrast +/-\n"
              << "  Z/X: Exposure +/-    | C/V: Gain +/-\n" << std::endl;
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

    // No post-processing effects - show raw video output
}

void Application::setupInputCallbacks()
{
    glfwSetWindowUserPointer(window_, this);
    glfwSetCursorPosCallback(window_, cursorPosCallback);
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    glfwSetScrollCallback(window_, scrollCallback);
    glfwSetKeyCallback(window_, keyCallback);
}

bool Application::updateVideoFrame()
{
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

    if (hasFrame && videoRenderer_) {
        videoRenderer_->updateFrame(currentTexture, currentSubIndex);
    }

    return hasFrame;
}

void Application::setupExportDecoder()
{
    if (exportDecoder_) return;

    exportDecoder_ = std::make_shared<Decoder>();
    exportDecoder_->open_video("D:/video/8K.mp4", [this](AVFrame* frame) {
        if (!allowExportDecoderUpdates_ || frame->format != AV_PIX_FMT_D3D11) return;

        std::lock_guard<std::mutex> lock(frameMutex_);
        frameData_.texture = (ID3D11Texture2D*)frame->data[0];
        frameData_.subIndex = (int)(intptr_t)frame->data[1];
        frameData_.hasNewFrame = true;

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    });
}

void Application::waitForFrames(int milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void Application::run()
{
    Decoder decoder;
    decoder.open_video("D:/video/8K.mp4", [this](AVFrame* frame) {
        if (!allowPlaybackDecoderUpdates_ || frame->format != AV_PIX_FMT_D3D11) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            return;
        }

        std::lock_guard<std::mutex> lock(frameMutex_);
        frameData_.texture = (ID3D11Texture2D*)frame->data[0];
        frameData_.subIndex = (int)(intptr_t)frame->data[1];
        frameData_.hasNewFrame = true;

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    });

    while(!glfwWindowShouldClose(window_)) {
        updateVideoFrame();
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

void Application::exportVideo()
{
    if (isExporting_) return;
    isExporting_ = true;

    VideoExportConfig config = {
        .outputPath = "exported_video.mp4",
        .width = width_,
        .height = height_,
        .fps = 60,
        .bitrate = 20000000,
        .codec = VideoCodec::H264,
        .preset = VideoQualityPreset::Fast,
        .hardwareAcceleration = true
    };

    VideoExporter exporter;
    if (!exporter.initialize(config, engine_.getDevice())) {
        std::cerr << "Failed to initialize exporter" << std::endl;
        isExporting_ = false;
        return;
    }

    exporter.setProgressCallback([](float p) {
        static int last = -1;
        int pct = (int)(p * 100);
        if (pct != last && pct % 5 == 0) {
            std::cout << "Export: " << pct << "%" << std::endl;
            last = pct;
        }
    });

    std::cout << "Exporting: " << config.width << "x" << config.height
              << " @ " << config.fps << " fps -> " << config.outputPath << std::endl;

    if (!exporter.beginExport(&engine_, 0.0f, 10.0f)) {
        std::cerr << "Failed to begin export" << std::endl;
        isExporting_ = false;
        return;
    }

    allowPlaybackDecoderUpdates_ = false;
    waitForFrames(100);

    allowExportDecoderUpdates_ = true;
    setupExportDecoder();
    waitForFrames(200);

    auto startTime = std::chrono::high_resolution_clock::now();

    while (!exporter.isFinished() && !exporter.isCancelled()) {
        updateVideoFrame();

        if (!exporter.exportFrame()) {
            std::cerr << "Export frame failed at " << exporter.getCurrentFrame() << std::endl;
            break;
        }

        glfwPollEvents();
        if (glfwWindowShouldClose(window_)) {
            exporter.cancel();
            break;
        }
    }

    allowExportDecoderUpdates_ = false;
    waitForFrames(200);

    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        frameData_ = {};
    }

    allowPlaybackDecoderUpdates_ = true;
    waitForFrames(100);

    if (!exporter.isCancelled() && exporter.finalize()) {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();
        std::cout << "Export done: " << exporter.getCurrentFrame()
                  << " frames in " << duration << "s" << std::endl;
    } else if (!exporter.isCancelled()) {
        std::cerr << "Export finalize failed" << std::endl;
    }

    isExporting_ = false;
}

void Application::switchRenderMode(VideoRenderer::RenderMode mode, float yaw, float pitch, float zoom)
{
    if (!videoRenderer_) return;

    videoRenderer_->setRenderMode(mode);
    yaw_ = yaw;
    pitch_ = pitch;
    zoom_ = zoom;
    videoRenderer_->setRotation(yaw_, pitch_);
    videoRenderer_->setZoom(zoom_);
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->engine_.setKeyState(key, action == GLFW_PRESS || action == GLFW_REPEAT);

    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;

            case GLFW_KEY_E:
                if (!app->isExporting_) {
                    std::cout << "Starting video export..." << std::endl;
                    app->exportVideo();
                }
                break;

            case GLFW_KEY_1:
                app->switchRenderMode(VideoRenderer::RenderMode::Planar, 0.0f, 0.0f, 1.0f);
                std::cout << "Switched to Planar mode" << std::endl;
                break;

            case GLFW_KEY_2:
                app->switchRenderMode(VideoRenderer::RenderMode::Panorama, 0.0f, 0.0f, 1.0f);
                std::cout << "Switched to Panorama mode" << std::endl;
                break;

            case GLFW_KEY_3:
                app->switchRenderMode(VideoRenderer::RenderMode::LittlePlanet, 0.0f, -1.57f, 0.8f);
                std::cout << "Switched to Little Planet mode" << std::endl;
                break;

            case GLFW_KEY_4:
                app->switchRenderMode(VideoRenderer::RenderMode::CrystalBall, 0.0f, 1.57f, 0.8f);
                std::cout << "Switched to Crystal Ball mode" << std::endl;
                break;

            // Brightness control
            case GLFW_KEY_Q:
                app->brightness_ += 0.05f;
                if (app->brightness_ > 1.0f) app->brightness_ = 1.0f;
                if (app->colorAdjust_) app->colorAdjust_->setParam("brightness", app->brightness_);
                std::cout << "Brightness: " << app->brightness_ << std::endl;
                break;
            case GLFW_KEY_W:
                app->brightness_ -= 0.05f;
                if (app->brightness_ < -1.0f) app->brightness_ = -1.0f;
                if (app->colorAdjust_) app->colorAdjust_->setParam("brightness", app->brightness_);
                std::cout << "Brightness: " << app->brightness_ << std::endl;
                break;

            // Contrast control
            case GLFW_KEY_A:
                app->contrast_ += 0.1f;
                if (app->contrast_ > 2.0f) app->contrast_ = 2.0f;
                if (app->colorAdjust_) app->colorAdjust_->setParam("contrast", app->contrast_);
                std::cout << "Contrast: " << app->contrast_ << std::endl;
                break;
            case GLFW_KEY_S:
                app->contrast_ -= 0.1f;
                if (app->contrast_ < 0.0f) app->contrast_ = 0.0f;
                if (app->colorAdjust_) app->colorAdjust_->setParam("contrast", app->contrast_);
                std::cout << "Contrast: " << app->contrast_ << std::endl;
                break;

            // Exposure control
            case GLFW_KEY_Z:
                app->exposure_ += 0.2f;
                if (app->exposure_ > 3.0f) app->exposure_ = 3.0f;
                if (app->colorAdjust_) app->colorAdjust_->setParam("exposure", app->exposure_);
                std::cout << "Exposure: " << app->exposure_ << " stops" << std::endl;
                break;
            case GLFW_KEY_X:
                app->exposure_ -= 0.2f;
                if (app->exposure_ < -3.0f) app->exposure_ = -3.0f;
                if (app->colorAdjust_) app->colorAdjust_->setParam("exposure", app->exposure_);
                std::cout << "Exposure: " << app->exposure_ << " stops" << std::endl;
                break;

            // Gain control
            case GLFW_KEY_C:
                app->gain_ += 0.1f;
                if (app->gain_ > 4.0f) app->gain_ = 4.0f;
                if (app->colorAdjust_) app->colorAdjust_->setParam("gain", app->gain_);
                std::cout << "Gain: " << app->gain_ << "x" << std::endl;
                break;
            case GLFW_KEY_V:
                app->gain_ -= 0.1f;
                if (app->gain_ < 0.0f) app->gain_ = 0.0f;
                if (app->colorAdjust_) app->colorAdjust_->setParam("gain", app->gain_);
                std::cout << "Gain: " << app->gain_ << "x" << std::endl;
                break;

            // Reset all adjustments
            case GLFW_KEY_R:
                app->brightness_ = 0.0f;
                app->contrast_ = 1.0f;
                app->saturation_ = 1.0f;
                app->exposure_ = 0.0f;
                app->gain_ = 1.0f;
                if (app->colorAdjust_) {
                    app->colorAdjust_->setParam("brightness", app->brightness_);
                    app->colorAdjust_->setParam("contrast", app->contrast_);
                    app->colorAdjust_->setParam("saturation", app->saturation_);
                    app->colorAdjust_->setParam("exposure", app->exposure_);
                    app->colorAdjust_->setParam("gain", app->gain_);
                }
                std::cout << "Reset all color adjustments" << std::endl;
                break;
        }
    }
}
