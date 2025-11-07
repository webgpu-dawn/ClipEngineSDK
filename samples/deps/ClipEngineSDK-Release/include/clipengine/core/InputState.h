#pragma once

#include <array>
#include <cstdint>

/**
 * @brief Input state for interactive shaders
 *
 * This structure contains all input information that can be used by shaders,
 * including mouse position, clicks, time, and custom data.
 */
struct InputState {
    // Mouse state
    struct Mouse {
        float x = 0.0f;           // Mouse X position (normalized 0-1)
        float y = 0.0f;           // Mouse Y position (normalized 0-1)
        float clickX = 0.0f;      // Last click X position (normalized 0-1)
        float clickY = 0.0f;      // Last click Y position (normalized 0-1)
        bool leftButton = false;   // Left button pressed
        bool rightButton = false;  // Right button pressed
        bool middleButton = false; // Middle button pressed
        float wheelDelta = 0.0f;   // Mouse wheel delta
    } mouse;

    // Keyboard state (simplified - for common keys)
    struct Keyboard {
        bool shift = false;
        bool ctrl = false;
        bool alt = false;
        bool space = false;
    } keyboard;

    // Time information
    struct Time {
        float elapsed = 0.0f;      // Total elapsed time in seconds
        float delta = 0.0f;         // Delta time since last frame
        uint32_t frameCount = 0;    // Frame counter
    } time;

    // Resolution information
    struct Resolution {
        uint32_t width = 1920;
        uint32_t height = 1080;
        float aspectRatio = 16.0f / 9.0f;
    } resolution;

    // Custom data that can be used by effects
    std::array<float, 4> custom = {0.0f, 0.0f, 0.0f, 0.0f};

    /**
     * @brief Update mouse position (pixel coordinates)
     * Automatically normalizes to 0-1 range
     */
    void setMousePosition(float pixelX, float pixelY) {
        mouse.x = pixelX / static_cast<float>(resolution.width);
        mouse.y = pixelY / static_cast<float>(resolution.height);
    }

    /**
     * @brief Update last click position
     */
    void setMouseClick(float pixelX, float pixelY) {
        mouse.clickX = pixelX / static_cast<float>(resolution.width);
        mouse.clickY = pixelY / static_cast<float>(resolution.height);
    }

    /**
     * @brief Update resolution
     */
    void setResolution(uint32_t w, uint32_t h) {
        resolution.width = w;
        resolution.height = h;
        resolution.aspectRatio = static_cast<float>(w) / static_cast<float>(h);
    }

    /**
     * @brief Reset per-frame state
     * Call this at the beginning of each frame
     */
    void resetPerFrameState() {
        mouse.wheelDelta = 0.0f;
    }
};
