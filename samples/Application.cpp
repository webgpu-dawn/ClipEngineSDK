#include "Application.h"
#include "VideoSource.h"

#include <iostream>
#include <thread>
#include <chrono>

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

    // Initialize PanoramaController
    panoramaController_.initialize(videoLayer_, colorEffect_);

    // Register all panorama interaction event listeners
    panoramaController_.registerInputListeners(inputSystem_);

    // Register Application-specific events (mode switching, export, exit, image loading)
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

    // Main application loop - supports export without exiting
    while(!glfwWindowShouldClose(window_)) {
        // Regular rendering loop
        while(!glfwWindowShouldClose(window_) && !shouldExport_) {
            // Skip normal rendering when exporting to avoid conflicts
            if (!isExporting_) {
                updateVideoFrame();
            }

            glfwPollEvents();

            // Update InputSystem (processes events and dispatches to listeners)
            // All input handling is now done via event listeners - no direct GLFW calls needed!
            inputSystem_.update(0.016); // ~60fps, can be replaced with actual deltaTime

            // Skip rendering when exporting (exporter handles its own rendering)
            if (!isExporting_) {
                // Render frame (all-in-one: updates, renders, presents)
                engine_.render();
            }
        }

        // If export was requested, exit main loop and perform export
        if (shouldExport_) {
            std::cout << "\n=== Exiting main loop to perform export ===\n" << std::endl;
            exportVideo();
            std::cout << "\n=== Export completed, resuming application ===\n" << std::endl;

            // Reset export flag to continue running
            shouldExport_ = false;

            // Wait a bit for the video source to stabilize after export
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        // If window was closed, exit the outer loop
        if (glfwWindowShouldClose(window_)) {
            break;
        }
    }

    glfwTerminate();
}

// ============================================================================
// InputSystem Event Handlers (Application-specific only)
// ============================================================================

void Application::exportVideo()
{
    if (isExporting_) return;

    // Ensure playback decoder is running and has frames before starting export
    std::cout << "Preparing for export, waiting for playback to stabilize..." << std::endl;
    int waitCount = 0;
    while (!videoSource_->hasNewFrame() && waitCount < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waitCount++;
    }

    if (!videoSource_->hasNewFrame()) {
        std::cerr << "Playback decoder not ready, cannot start export" << std::endl;
        return;
    }

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

    // Wait for export decoder to produce first frame before starting export
    std::cout << "Waiting for first frame from export decoder..." << std::endl;
    int exportWaitCount = 0;
    while (!videoSource_->hasNewFrame() && exportWaitCount < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        exportWaitCount++;
    }

    if (!videoSource_->hasNewFrame()) {
        std::cerr << "Export decoder failed to produce frames" << std::endl;
        videoSource_->endExportMode();
        isExporting_ = false;
        return;
    }

    // Load the first frame
    updateVideoFrame();
    std::cout << "First frame ready, starting export..." << std::endl;

    // Perform a dummy render to ensure all GPU states are synchronized
    // This helps transition from window rendering to offscreen rendering
    std::cout << "Synchronizing render state..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto startTime = std::chrono::steady_clock::now();

    // Export loop - process window events but don't update input system
    while (!exporter.isFinished() && !exporter.isCancelled()) {
        // Update video frame from export decoder
        updateVideoFrame();

        // Export the frame
        if (!exporter.exportFrame()) {
            std::cerr << "Export frame failed at " << exporter.getCurrentFrame() << std::endl;
            break;
        }

        // Only process window events, no input system updates during export
        glfwPollEvents();
        if (glfwWindowShouldClose(window_)) {
            exporter.cancel();
            break;
        }

        // Small delay to prevent overwhelming the system
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
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

    // Handle Application-specific key press actions
    switch (event.keyCode) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
            break;

        case GLFW_KEY_E:
            if (!isExporting_ && !shouldExport_) {
                std::cout << "Export requested, will start after current frame..." << std::endl;
                shouldExport_ = true;
            }
            break;

        case GLFW_KEY_1:
            // Switch to Planar mode
            videoLayer_->setRenderMode(VideoRenderer::RenderMode::Planar);
            std::cout << "Switched to Planar mode" << std::endl;
            break;

        case GLFW_KEY_2:
            // Switch to Panorama mode
            videoLayer_->setRenderMode(VideoRenderer::RenderMode::Panorama);
            std::cout << "Switched to Equirectangular (360°) mode" << std::endl;
            break;

        case GLFW_KEY_3:
            // Switch to Little Planet mode
            videoLayer_->setRenderMode(VideoRenderer::RenderMode::LittlePlanet);
            std::cout << "Switched to Little Planet mode" << std::endl;
            break;

        case GLFW_KEY_4:
            // Switch to Crystal Ball mode
            videoLayer_->setRenderMode(VideoRenderer::RenderMode::CrystalBall);
            std::cout << "Switched to Crystal Ball mode" << std::endl;
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

void Application::loadImageTexture(const std::string& imagePath)
{
    std::cout << "\n[ImageLayer] Loading image: " << imagePath << std::endl;

    if (!imageLayer_) {
        std::cerr << "[ImageLayer] Layer not initialized" << std::endl;
        return;
    }

    // Load image using SDK's built-in loadFromFile method
    auto* videoRenderer = static_cast<VideoRenderer*>(imageLayer_);
    if (videoRenderer && videoRenderer->loadFromFile(imagePath)) {
        imageLayer_->setEnabled(true);
        std::cout << "[ImageLayer] Image loaded successfully" << std::endl;
        std::cout << "[ImageLayer] Press 'I' to toggle visibility" << std::endl;
    } else {
        std::cerr << "[ImageLayer] Failed to load image from: " << imagePath << std::endl;
    }
}

