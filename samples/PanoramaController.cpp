#include "PanoramaController.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>

PanoramaController::PanoramaController()
{
}

void PanoramaController::initialize(VideoLayer* videoRenderer, ShaderEffect* colorEffect)
{
    videoRenderer_ = videoRenderer;
    colorEffect_ = colorEffect;

    // Reset to initial state
    yaw_ = 0.0f;
    pitch_ = 0.0f;
    zoom_ = 1.0f;
    dragging_ = false;

    // Reset color adjustments
    brightness_ = 1.0f;
    contrast_ = 1.0f;
    saturation_ = 1.0f;
    exposure_ = 0.0f;
    gain_ = 1.0f;
}

void PanoramaController::registerInputListeners(clipengine::InputSystem& inputSystem)
{
    using namespace clipengine;

    // Register pointer events
    inputSystem.addEventListener(InputEventType::PointerDown,
        [this](const InputEvent& e) { handlePointerDown(e); });

    inputSystem.addEventListener(InputEventType::PointerMove,
        [this](const InputEvent& e) { handlePointerMove(e); });

    inputSystem.addEventListener(InputEventType::PointerUp,
        [this](const InputEvent& e) { handlePointerUp(e); });

    inputSystem.addEventListener(InputEventType::Scroll,
        [this](const InputEvent& e) { handleScroll(e); });

    // Register keyboard events for color adjustments
    inputSystem.addEventListener(InputEventType::KeyDown,
        [this](const InputEvent& e) { handleKeyDown(e); });
}

void PanoramaController::handlePointerDown(const clipengine::InputEvent& event)
{
    using namespace clipengine;

    // Left mouse button starts dragging for panorama rotation
    if (event.button == MouseButton::Left && videoRenderer_) {
        dragging_ = true;
        lastMouseX_ = event.mouseX;
        lastMouseY_ = event.mouseY;
    }
}

void PanoramaController::handlePointerMove(const clipengine::InputEvent& event)
{
    // Handle panorama drag rotation
    if (dragging_ && videoRenderer_) {
        float dx = static_cast<float>(event.mouseX - lastMouseX_);
        float dy = static_cast<float>(event.mouseY - lastMouseY_);

        // Standard panorama controls (Google Street View style)
        // Drag right -> scene moves right -> look left (natural drag feel)
        // Drag down -> scene moves down -> look up
        yaw_ -= dx * 0.005f;
        pitch_ += dy * 0.005f;

        // Clamp pitch to prevent flipping
        pitch_ = std::clamp(pitch_, -1.5f, 1.5f);

        videoRenderer_->setRotation(yaw_, pitch_);
        lastMouseX_ = event.mouseX;
        lastMouseY_ = event.mouseY;
    }
}

void PanoramaController::handlePointerUp(const clipengine::InputEvent& event)
{
    using namespace clipengine;

    // End dragging
    if (event.button == MouseButton::Left) {
        dragging_ = false;
    }
}

void PanoramaController::handleScroll(const clipengine::InputEvent& event)
{
    // Handle zoom for panorama mode
    if (videoRenderer_) {
        zoom_ += event.deltaY * 0.1f;
        zoom_ = std::clamp(zoom_, 0.1f, 5.0f);
        videoRenderer_->setZoom(zoom_);
    }
}

void PanoramaController::handleKeyDown(const clipengine::InputEvent& event)
{
    // Color adjustment shortcuts
    switch (event.keyCode) {
        case GLFW_KEY_Q: adjustBrightness(0.1f);  break;
        case GLFW_KEY_W: adjustBrightness(-0.1f); break;
        case GLFW_KEY_A: adjustContrast(0.1f);    break;
        case GLFW_KEY_S: adjustContrast(-0.1f);   break;
        case GLFW_KEY_Z: adjustExposure(0.2f);    break;
        case GLFW_KEY_X: adjustExposure(-0.2f);   break;
        case GLFW_KEY_C: adjustGain(0.1f);        break;
        case GLFW_KEY_V: adjustGain(-0.1f);       break;
        case GLFW_KEY_R: resetColorAdjustments(); break;
    }
}

void PanoramaController::adjustBrightness(float delta)
{
    brightness_ += delta;
    brightness_ = std::clamp(brightness_, 0.0f, 2.0f);

    if (colorEffect_) {
        colorEffect_->setParam("brightness", brightness_ - 1.0f);
    }
    std::cout << "Brightness: " << brightness_ << std::endl;
}

void PanoramaController::adjustContrast(float delta)
{
    contrast_ += delta;
    contrast_ = std::clamp(contrast_, 0.0f, 2.0f);

    if (colorEffect_) {
        colorEffect_->setParam("contrast", contrast_);
    }
    std::cout << "Contrast: " << contrast_ << std::endl;
}

void PanoramaController::adjustExposure(float delta)
{
    exposure_ += delta;
    exposure_ = std::clamp(exposure_, -3.0f, 3.0f);

    if (colorEffect_) {
        colorEffect_->setParam("exposure", exposure_);
    }
    std::cout << "Exposure: " << exposure_ << " stops" << std::endl;
}

void PanoramaController::adjustGain(float delta)
{
    gain_ += delta;
    gain_ = std::clamp(gain_, 0.0f, 4.0f);

    if (colorEffect_) {
        colorEffect_->setParam("gain", gain_);
    }
    std::cout << "Gain: " << gain_ << "x" << std::endl;
}

void PanoramaController::resetColorAdjustments()
{
    brightness_ = 1.0f;
    contrast_ = 1.0f;
    saturation_ = 1.0f;
    exposure_ = 0.0f;
    gain_ = 1.0f;

    if (colorEffect_) {
        colorEffect_->setParam("brightness", 0.0f);
        colorEffect_->setParam("contrast", 1.0f);
        colorEffect_->setParam("saturation", 1.0f);
        colorEffect_->setParam("exposure", 0.0f);
        colorEffect_->setParam("gain", 1.0f);
    }
    std::cout << "Color adjustments reset to defaults" << std::endl;
}
