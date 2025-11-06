#include "Application.h"
#include "VideoSource.h"
#include "ImageLoader.h"

#include <iostream>

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

#ifdef CLIPENGINE_DEBUG_WINDOW_ENABLED
    // Initialize Debug Window (standalone window)
    if (!debugWindow_.initialize(&engine_, "ClipEngine Debug")) {
        std::cerr << "Failed to initialize Debug Window" << std::endl;
        return;
    }

    // Attach debug window to engine for automatic updates
    engine_.setDebugWindow(&debugWindow_);
#endif

    // Create color adjustment effect (using new API naming)
    auto colorAdjust = ShaderEffect::createColorAdjust();
    colorAdjust->setParam("brightness", 0.0f);
    colorAdjust->setParam("contrast", 1.0f);
    colorAdjust->setParam("saturation", 1.0f);
    colorAdjust->setParam("exposure", 0.0f);
    colorAdjust->setParam("gain", 1.0f);
    colorAdjust->setParam("hue", 0.0f);
    colorEffect_ = colorAdjust.get();
    engine_.getGlobalFilterChain().addFilter(std::move(colorAdjust));

    // Setup video source
    videoSource_ = std::make_unique<VideoSource>();
    videoSource_->open("D:/video/test.mp4");

    std::cout << "ClipEngine initialized\nControls:\n"
              << "  1-4: Switch render modes | E: Export video | ESC: Exit\n"
              << "  L: Load image overlay   | I: Toggle image visibility\n"
              << "  Mouse: Drag to rotate, wheel to zoom\n"
              << "  Q/W: Brightness +/-  | A/S: Contrast +/-\n"
              << "  Z/X: Exposure +/-    | C/V: Gain +/-    | R: Reset\n" << std::endl;

    initialized_ = true;
}

void Application::setupScene()
{
    // ========================================================================
    // Layer + Effect API
    // ========================================================================
    std::cout << "\n=== Layer + Effect API ===" << std::endl;
    // Create main video layer
    auto video = std::make_unique<VideoRenderer>();
    video->setTransform(0.0f, 0.0f, 1.0f, 1.0f);
    video->setLayer(0);
    video->setName("Main Video Layer");
    video->setAspect((float)width_ / (float)height_);
    size_t videoIdx = engine_.addLayer(std::move(video));

    videoLayer_ = static_cast<VideoRenderer*>(engine_.getLayer(videoIdx));

    if (videoLayer_) {
        videoLayer_->setRenderMode(VideoRenderer::RenderMode::Panorama);
        std::cout << "[VideoLayer] Initialized" << std::endl;
    }

    // Create image overlay layer
    auto imageLayer = std::make_unique<VideoRenderer>(VideoFormat::RGBA);
    imageLayer->setTransform(0.0f, 0.0f, 0.2f, 0.2f);  // Full screen
    imageLayer->setLayer(1);  // Above video layer
    imageLayer->setName("Image Overlay Layer");
    imageLayer->setEnabled(true);  // Start hidden
    size_t imageIdx = engine_.addLayer(std::move(imageLayer));

    imageLayer_ = static_cast<TextureRenderer*>(engine_.getLayer(imageIdx));
    std::cout << "[ImageLayer] Initialized" << std::endl;

    if (colorEffect_) {
        std::cout << "[ColorAdjustEffect] Initialized" << std::endl;
    }

    std::cout << "=================================\n" << std::endl;
}

void Application::setupInputCallbacks()
{
    // Initialize InputSystem (window-system independent)
    if (!inputSystem_.initialize(width_, height_)) {
        std::cerr << "Failed to initialize InputSystem" << std::endl;
        return;
    }

    // Create and attach GLFW adapter (handles all GLFW input events)
    inputAdapter_ = std::make_unique<clipengine::InputSystemGLFWAdapter>(&inputSystem_);
    inputAdapter_->attach(window_);

    // Setup event listeners for application logic
    setupInputEventListeners();

    std::cout << "InputSystem initialized with GLFW adapter and event listeners" << std::endl;
}

void Application::setupInputEventListeners()
{
    using namespace clipengine;

    // Pointer Down - start dragging or handle clicks
    inputSystem_.addEventListener(InputEventType::PointerDown,
        [this](const InputEvent& event) {
            handlePointerDown(event);
        }
    );

    // Pointer Move - handle drag rotation
    inputSystem_.addEventListener(InputEventType::PointerMove,
        [this](const InputEvent& event) {
            handlePointerMove(event);
        }
    );

    // Pointer Up - end dragging
    inputSystem_.addEventListener(InputEventType::PointerUp,
        [this](const InputEvent& event) {
            handlePointerUp(event);
        }
    );

    // Scroll - handle zoom
    inputSystem_.addEventListener(InputEventType::Scroll,
        [this](const InputEvent& event) {
            handleScroll(event);
        }
    );

    // Key Down - handle keyboard controls
    inputSystem_.addEventListener(InputEventType::KeyDown,
        [this](const InputEvent& event) {
            handleKeyDown(event);
        }
    );
}

void Application::updateVideoFrame()
{
    // Update video layer with latest frame
    if (videoSource_->hasNewFrame() && videoLayer_) {
        auto frame = videoSource_->getLatestFrame();
        videoLayer_->updateFrame(frame.texture, frame.subIndex);
    }
}


void Application::run()
{
    if (!initialized_) {
        std::cerr << "Application not initialized! Cannot run." << std::endl;
        return;
    }

    while(!glfwWindowShouldClose(window_)) {
        updateVideoFrame();
        glfwPollEvents();

        // Update InputSystem (processes events and dispatches to listeners)
        // All input handling is now done via event listeners - no direct GLFW calls needed!
        inputSystem_.update(0.016); // ~60fps, can be replaced with actual deltaTime

        // Render frame (all-in-one: updates, renders, presents)
        engine_.render();
    }

    glfwTerminate();
}

// ============================================================================
// InputSystem Event Handlers (replaces GLFW callbacks)
// ============================================================================

void Application::handlePointerDown(const clipengine::InputEvent& event)
{
    using namespace clipengine;

    // Left mouse button starts dragging for panorama rotation
    if (event.button == MouseButton::Left && videoLayer_) {
        dragging_ = true;
        lastMouseX_ = event.mouseX;
        lastMouseY_ = event.mouseY;
    }

    // Sync with engine's input state (for backward compatibility)
    engine_.setMouseButton(static_cast<int>(event.button), true);
}

void Application::handlePointerMove(const clipengine::InputEvent& event)
{
    // Sync mouse position with engine (for backward compatibility)
    engine_.setMousePosition(event.mouseX, event.mouseY);

    // Handle panorama drag rotation
    if (dragging_ && videoLayer_) {
        float dx = event.mouseX - lastMouseX_;
        float dy = event.mouseY - lastMouseY_;

        yaw_ += dx * 0.005f;
        pitch_ += dy * 0.005f;

        // Clamp pitch to prevent flipping
        if (pitch_ > 1.5f) pitch_ = 1.5f;
        if (pitch_ < -1.5f) pitch_ = -1.5f;

        // TEMPORARY: Use old VideoRenderer
        videoLayer_->setRotation(yaw_, pitch_);

        // TODO: Implement VideoLayer::setRotation()
        // videoLayer_->setRotation(yaw_, pitch_);

        lastMouseX_ = event.mouseX;
        lastMouseY_ = event.mouseY;
    }
}

void Application::handlePointerUp(const clipengine::InputEvent& event)
{
    using namespace clipengine;

    // End dragging
    if (event.button == MouseButton::Left) {
        dragging_ = false;
    }

    // Sync with engine's input state (for backward compatibility)
    engine_.setMouseButton(static_cast<int>(event.button), false);
}

void Application::handleScroll(const clipengine::InputEvent& event)
{
    // Sync with engine (for backward compatibility)
    engine_.setMouseWheel(event.deltaY);

    // Handle zoom for panorama mode
    if (videoLayer_) {
        zoom_ += event.deltaY * 0.1f;
        if (zoom_ < 0.1f) zoom_ = 0.1f;
        if (zoom_ > 5.0f) zoom_ = 5.0f;

        // TEMPORARY: Use old VideoRenderer
        videoLayer_->setZoom(zoom_);

        // TODO: Implement VideoLayer::setZoom()
        // videoLayer_->setZoom(zoom_);
    }
}

void Application::exportVideo()
{
    if (isExporting_) return;
    isExporting_ = true;

    // Get actual video duration and frame rate
    float videoDuration = static_cast<float>(videoSource_->getDuration());
    if (videoDuration <= 0.0f) {
        std::cerr << "Cannot determine video duration, using default 10s" << std::endl;
        videoDuration = 10.0f;
    }

    uint32_t videoFps = static_cast<uint32_t>(videoSource_->getFrameRate());
    if (videoFps <= 0) {
        std::cerr << "Cannot determine video frame rate, using default 30fps" << std::endl;
        videoFps = 30;
    }

    VideoExportConfig config = {
        .outputPath = "exported_video.mp4",
        .width = width_,
        .height = height_,
        .fps = videoFps,  // Use source video's frame rate
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
              << " @ " << config.fps << " fps, duration: " << videoDuration << "s -> " << config.outputPath << std::endl;

    if (!exporter.beginExport(&engine_, 0.0f, videoDuration)) {
        std::cerr << "Failed to begin export" << std::endl;
        isExporting_ = false;
        return;
    }

    // Switch video source to export mode
    videoSource_->beginExportMode();

    auto startTime = std::chrono::steady_clock::now();

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

    // Switch back to playback mode
    videoSource_->endExportMode();

    if (!exporter.isCancelled() && exporter.finalize()) {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime).count();
        std::cout << "Export done: " << exporter.getCurrentFrame()
                  << " frames in " << duration << "s" << std::endl;
    } else if (!exporter.isCancelled()) {
        std::cerr << "Export finalize failed" << std::endl;
    }

    isExporting_ = false;
}

void Application::handleKeyDown(const clipengine::InputEvent& event)
{
    // Sync with engine's input state (for backward compatibility)
    engine_.setKeyState(event.keyCode, true);

    // Handle key press actions
    switch (event.keyCode) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
            break;

        case GLFW_KEY_E:
            if (!isExporting_) {
                std::cout << "Starting video export..." << std::endl;
                exportVideo();
            }
            break;

        case GLFW_KEY_1:
            switchRenderMode(VideoRenderer::RenderMode::Planar, 0.0f, 0.0f, 1.0f);
            std::cout << "Switched to Planar mode" << std::endl;
            break;

        case GLFW_KEY_2:
            switchRenderMode(VideoRenderer::RenderMode::Panorama, 0.0f, 0.0f, 1.0f);
            std::cout << "Switched to Equirectangular (360°) mode" << std::endl;
            break;

        case GLFW_KEY_3:
            switchRenderMode(VideoRenderer::RenderMode::LittlePlanet, 0.0f, -1.57f, 0.8f);
            std::cout << "Switched to Little Planet mode" << std::endl;
            break;

        case GLFW_KEY_4:
            switchRenderMode(VideoRenderer::RenderMode::CrystalBall, 0.0f, 1.57f, 0.8f);
            std::cout << "Switched to Crystal Ball mode" << std::endl;
            break;

        // Brightness control (0.0 - 2.0, 1.0 = normal)
        case GLFW_KEY_Q:
            brightness_ += 0.1f;
            if (brightness_ > 2.0f) brightness_ = 2.0f;
            if (colorEffect_) {
                colorEffect_->setParam("brightness", (brightness_ - 1.0f));  // Convert to -1.0~1.0
            }
            std::cout << "Brightness: " << brightness_ << std::endl;
            break;

        case GLFW_KEY_W:
            brightness_ -= 0.1f;
            if (brightness_ < 0.0f) brightness_ = 0.0f;
            if (colorEffect_) {
                colorEffect_->setParam("brightness", (brightness_ - 1.0f));
            }
            std::cout << "Brightness: " << brightness_ << std::endl;
            break;

        // Contrast control
        case GLFW_KEY_A:
            contrast_ += 0.1f;
            if (contrast_ > 2.0f) contrast_ = 2.0f;
            if (colorEffect_) {
                colorEffect_->setParam("contrast", contrast_);
            }
            std::cout << "Contrast: " << contrast_ << std::endl;
            break;

        case GLFW_KEY_S:
            contrast_ -= 0.1f;
            if (contrast_ < 0.0f) contrast_ = 0.0f;
            if (colorEffect_) {
                colorEffect_->setParam("contrast", contrast_);
            }
            std::cout << "Contrast: " << contrast_ << std::endl;
            break;

        // Exposure control
        case GLFW_KEY_Z:
            exposure_ += 0.2f;
            if (exposure_ > 3.0f) exposure_ = 3.0f;
            if (colorEffect_) {
                colorEffect_->setParam("exposure", exposure_);
            }
            std::cout << "Exposure: " << exposure_ << " stops" << std::endl;
            break;

        case GLFW_KEY_X:
            exposure_ -= 0.2f;
            if (exposure_ < -3.0f) exposure_ = -3.0f;
            if (colorEffect_) {
                colorEffect_->setParam("exposure", exposure_);
            }
            std::cout << "Exposure: " << exposure_ << " stops" << std::endl;
            break;

        // Gain control
        case GLFW_KEY_C:
            gain_ += 0.1f;
            if (gain_ > 4.0f) gain_ = 4.0f;
            if (colorEffect_) {
                colorEffect_->setParam("gain", gain_);
            }
            std::cout << "Gain: " << gain_ << "x" << std::endl;
            break;

        case GLFW_KEY_V:
            gain_ -= 0.1f;
            if (gain_ < 0.0f) gain_ = 0.0f;
            if (colorEffect_) {
                colorEffect_->setParam("gain", gain_);
            }
            std::cout << "Gain: " << gain_ << "x" << std::endl;
            break;

        // Reset all color adjustments
        case GLFW_KEY_R:
            brightness_ = 1.0f;
            contrast_ = 1.0f;
            saturation_ = 1.0f;
            exposure_ = 0.0f;
            gain_ = 1.0f;
            if (colorEffect_) {
                colorEffect_->setParam("brightness", 0.0f);   // 1.0 -> 0.0 (normalized)
                colorEffect_->setParam("contrast", 1.0f);
                colorEffect_->setParam("saturation", 1.0f);
                colorEffect_->setParam("exposure", 0.0f);
                colorEffect_->setParam("gain", 1.0f);
            }
            std::cout << "Reset all color adjustments" << std::endl;
            break;

        // Load image overlay
        case GLFW_KEY_L:
            // Load a sample image (you can change this path)
            loadImageTexture("D:/video/overlay.JPG");
            break;

        // Toggle image layer visibility
        case GLFW_KEY_I:
            if (imageLayer_) {
                bool isVisible = imageLayer_->isEnabled();
                imageLayer_->setEnabled(!isVisible);
                std::cout << "Image layer: " << (!isVisible ? "visible" : "hidden") << std::endl;
            }
            break;
    }
}

void Application::switchRenderMode(VideoRenderer::RenderMode mode, float yaw, float pitch, float zoom)
{
    if (!videoLayer_) return;

    videoLayer_->setRenderMode(mode);
    yaw_ = yaw;
    pitch_ = pitch;
    zoom_ = zoom;
    videoLayer_->setRotation(yaw_, pitch_);
    videoLayer_->setZoom(zoom_);
}

void Application::loadImageTexture(const std::string& imagePath)
{
    std::cout << "\n[ImageLayer] Loading image: " << imagePath << std::endl;

    if (!imageLayer_) {
        std::cerr << "[ImageLayer] Layer not initialized" << std::endl;
        return;
    }

    // Load image using ImageLoader
    ComPtr<ID3D11Texture2D> texture;
    uint32_t width, height;
    if (!ImageLoader::loadImage(imagePath, texture, width, height)) {
        std::cerr << "[ImageLayer] Failed to load image from: " << imagePath << std::endl;
        return;
    }

    // Update texture in the image layer
    auto* videoRenderer = static_cast<VideoRenderer*>(imageLayer_);
    if (videoRenderer) {
        videoRenderer->updateFrame(texture.Get(), 0);
        imageLayer_->setEnabled(true);
        std::cout << "[ImageLayer] Loaded successfully: " << width << "x" << height << std::endl;
        std::cout << "[ImageLayer] Press 'I' to toggle visibility" << std::endl;
    } else {
        std::cerr << "[ImageLayer] Failed to update texture" << std::endl;
    }
}
