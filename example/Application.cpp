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

    std::cout << "ClipEngine initialized\nControls:\n"
              << "  1-4: Switch render modes (Planar/Panorama/Little Planet/Crystal Ball)\n"
              << "  E: Export video (10s) | ESC: Exit\n"
              << "  Mouse: Drag to rotate, wheel to zoom\n" << std::endl;
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

void Application::run()
{
    Decoder decoder;
    decoder.open_video("D:/video/8K.mp4", [&, this](AVFrame* frame) {
        // Only update frames when playback is active (not during export)
        if (!allowPlaybackDecoderUpdates_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            return;
        }

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

void Application::exportVideo()
{
    if (isExporting_) {
        std::cout << "Export already in progress" << std::endl;
        return;
    }

    isExporting_ = true;

    // Configure export settings
    VideoExportConfig config;
    config.outputPath = "exported_video.mp4";
    config.width = width_;
    config.height = height_;
    config.fps = 60;
    config.bitrate = 20000000;  // 20 Mbps for high quality
    config.codec = VideoCodec::H264;
    config.preset = VideoQualityPreset::Fast;
    config.hardwareAcceleration = true;

    // Create and initialize exporter
    VideoExporter exporter;
    if (!exporter.initialize(config, engine_.getDevice())) {
        std::cerr << "Failed to initialize video exporter" << std::endl;
        isExporting_ = false;
        return;
    }

    // Set progress callback
    exporter.setProgressCallback([](float progress) {
        static int lastPercent = -1;
        int percent = static_cast<int>(progress * 100);
        if (percent != lastPercent && percent % 5 == 0) {
            std::cout << "Export progress: " << percent << "%" << std::endl;
            lastPercent = percent;
        }
    });

    // Begin export (10 seconds)
    std::cout << "Exporting to: " << config.outputPath
              << " (" << config.width << "x" << config.height << " @ " << config.fps << " fps)" << std::endl;

    if (!exporter.beginExport(&engine_, 0.0f, 10.0f)) {
        std::cerr << "Failed to begin export" << std::endl;
        isExporting_ = false;
        return;
    }

    // Pause playback decoder and enable export decoder
    allowPlaybackDecoderUpdates_ = false;  // Pause normal playback
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Let playback decoder stop

    allowExportDecoderUpdates_ = true;  // Enable export decoder

    // Create or reuse export decoder
    if (!exportDecoder_) {
        exportDecoder_ = std::make_shared<Decoder>();
        exportDecoder_->open_video("D:/video/8K.mp4", [this](AVFrame* frame) {
            // Only update frames when export is active
            if (!allowExportDecoderUpdates_) {
                return;
            }

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
    }

    // Wait for decoder to start
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Export all frames
    auto startTime = std::chrono::high_resolution_clock::now();

    while (!exporter.isFinished() && !exporter.isCancelled()) {
        // Update video frame from decoder
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

        // Update frame in renderer
        if (hasFrame && videoRenderer_) {
            videoRenderer_->updateFrame(currentTexture, currentSubIndex);
        }

        // Export frame
        if (!exporter.exportFrame()) {
            std::cerr << "Failed to export frame " << exporter.getCurrentFrame() << std::endl;
            break;
        }

        // Poll events to keep the window responsive
        glfwPollEvents();

        // Check if user wants to cancel
        if (glfwWindowShouldClose(window_)) {
            std::cout << "Export cancelled by user" << std::endl;
            exporter.cancel();
            break;
        }
    }

    // Stop export decoder and resume playback
    allowExportDecoderUpdates_ = false;

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        frameData_.texture = nullptr;
        frameData_.subIndex = 0;
        frameData_.hasNewFrame = false;
    }

    allowPlaybackDecoderUpdates_ = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Finalize export
    if (!exporter.isCancelled() && exporter.finalize()) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
        std::cout << "Export completed: " << exporter.getCurrentFrame() << " frames in "
                  << duration << "s -> " << config.outputPath << std::endl;
    } else if (!exporter.isCancelled()) {
        std::cerr << "Failed to finalize export" << std::endl;
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
        }
    }
}
