#pragma once

#include <clipengine/layers/VideoRenderer.h>
#include <clipengine/effects/ShaderEffect.h>
#include <clipengine/input/InputSystem.h>

/**
 * @brief Controller for panorama video interaction
 *
 * Handles mouse/keyboard input for:
 * - Panorama rotation (drag to rotate)
 * - Zoom (mouse wheel)
 * - Color adjustments (keyboard shortcuts)
 */
class PanoramaController
{
public:
    PanoramaController();
    ~PanoramaController() = default;

    /**
     * @brief Initialize with video renderer and color effect
     * @param videoRenderer The video renderer to control
     * @param colorEffect The color effect for adjustments (optional)
     */
    void initialize(VideoRenderer* videoRenderer, ShaderEffect* colorEffect = nullptr);

    /**
     * @brief Register input event listeners
     * @param inputSystem The input system to register with
     */
    void registerInputListeners(clipengine::InputSystem& inputSystem);

private:
    // Input event handlers
    void handlePointerDown(const clipengine::InputEvent& event);
    void handlePointerMove(const clipengine::InputEvent& event);
    void handlePointerUp(const clipengine::InputEvent& event);
    void handleScroll(const clipengine::InputEvent& event);
    void handleKeyDown(const clipengine::InputEvent& event);

    // Color adjustment helpers
    void adjustBrightness(float delta);
    void adjustContrast(float delta);
    void adjustExposure(float delta);
    void adjustGain(float delta);
    void resetColorAdjustments();

    // Controlled objects
    VideoRenderer* videoRenderer_ = nullptr;
    ShaderEffect* colorEffect_ = nullptr;

    // Interaction state
    bool dragging_ = false;
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;

    // Panorama rotation state
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float zoom_ = 1.0f;

    // Color adjustment parameters (1.0 = normal)
    float brightness_ = 1.0f;
    float contrast_ = 1.0f;
    float saturation_ = 1.0f;
    float exposure_ = 0.0f;
    float gain_ = 1.0f;
};
