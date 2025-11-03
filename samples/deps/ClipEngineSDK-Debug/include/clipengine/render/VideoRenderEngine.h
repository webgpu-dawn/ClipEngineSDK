#pragma once

#include "../common/Common.h"
#include "../core/CeRenderable.h"
#include "FilterChain.h"
#include "InputState.h"
#include <vector>
#include <memory>
#include <algorithm>

/**
 * @brief Main rendering engine for video composition
 *
 * VideoRenderEngine manages multiple renderable objects (images, text, effects)
 * and composites them into a final output. Each renderable can have its own
 * filter chain for post-processing effects.
 *
 * Features:
 * - Multiple layers with z-ordering
 * - Per-object filter chains
 * - Global post-processing filters
 * - Efficient rendering with minimal texture copies
 */
class VideoRenderEngine {
public:
    VideoRenderEngine() = default;
    ~VideoRenderEngine() = default;

    /**
     * @brief Initialize the render engine
     * @param device WebGPU device
     * @param format Output texture format
     * @param width Output width
     * @param height Output height
     */
    bool initialize(wgpu::Device device, wgpu::TextureFormat format, uint32_t width, uint32_t height);

    /**
     * @brief Add a renderable object to the engine
     * @param renderable Unique pointer to renderable (ownership transferred)
     * @return Index of the added renderable
     */
    size_t addRenderable(std::unique_ptr<CeRenderable> renderable);

    /**
     * @brief Remove a renderable by index
     */
    void removeRenderable(size_t index);

    /**
     * @brief Get renderable at index
     */
    CeRenderable* getRenderable(size_t index);

    /**
     * @brief Get number of renderables
     */
    size_t getRenderableCount() const { return renderables_.size(); }

    /**
     * @brief Clear all renderables
     */
    void clearRenderables();

    /**
     * @brief Render all objects to the given texture view
     * @param outputView Target texture view
     */
    void render(wgpu::TextureView outputView);

    /**
     * @brief Update all renderables
     * @param deltaTime Time delta in seconds
     */
    void update(float deltaTime);

    /**
     * @brief Resize the render engine output
     */
    void resize(uint32_t width, uint32_t height);

    /**
     * @brief Get global post-processing filter chain
     * Filters added here are applied to the final composited output
     */
    FilterChain& getGlobalFilterChain() { return globalFilterChain_; }

    /**
     * @brief Get filter chain for a specific renderable
     * @param index Renderable index
     * @return Pointer to filter chain, or nullptr if not supported
     */
    FilterChain* getRenderableFilterChain(size_t index);

    /**
     * @brief Set clear color
     */
    void setClearColor(float r, float g, float b, float a) {
        clearColor_ = {r, g, b, a};
    }

    /**
     * @brief Get input state for reading/writing
     */
    InputState& getInputState() { return inputState_; }
    const InputState& getInputState() const { return inputState_; }

    /**
     * @brief Update mouse position (in pixel coordinates)
     */
    void setMousePosition(float x, float y) {
        inputState_.setMousePosition(x, y);
    }

    /**
     * @brief Update mouse button state
     */
    void setMouseButton(int button, bool pressed) {
        if (button == 0) inputState_.mouse.leftButton = pressed;
        else if (button == 1) inputState_.mouse.rightButton = pressed;
        else if (button == 2) inputState_.mouse.middleButton = pressed;

        if (pressed) {
            inputState_.setMouseClick(
                inputState_.mouse.x * inputState_.resolution.width,
                inputState_.mouse.y * inputState_.resolution.height
            );
        }
    }

    /**
     * @brief Update mouse wheel
     */
    void setMouseWheel(float delta) {
        inputState_.mouse.wheelDelta = delta;
    }

    /**
     * @brief Update keyboard state
     */
    void setKeyState(int key, bool pressed) {
        // Map common keys (can be extended)
        if (key == 340 || key == 344) inputState_.keyboard.shift = pressed;  // GLFW_KEY_LEFT_SHIFT / RIGHT_SHIFT
        if (key == 341 || key == 345) inputState_.keyboard.ctrl = pressed;   // GLFW_KEY_LEFT_CONTROL / RIGHT_CONTROL
        if (key == 342 || key == 346) inputState_.keyboard.alt = pressed;    // GLFW_KEY_LEFT_ALT / RIGHT_ALT
        if (key == 32) inputState_.keyboard.space = pressed;                 // GLFW_KEY_SPACE
    }

private:
    struct RenderableEntry {
        std::unique_ptr<CeRenderable> renderable;
        FilterChain filterChain;
        wgpu::Texture intermediateTexture;
        wgpu::TextureView intermediateView;
        bool needsIntermediateTexture = false;

        // Allow move construction and assignment
        RenderableEntry() = default;
        RenderableEntry(RenderableEntry&&) = default;
        RenderableEntry& operator=(RenderableEntry&&) = default;
        // Disable copy
        RenderableEntry(const RenderableEntry&) = delete;
        RenderableEntry& operator=(const RenderableEntry&) = delete;
    };

    wgpu::Device device_;
    wgpu::TextureFormat format_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    std::vector<RenderableEntry> renderables_;
    FilterChain globalFilterChain_;

    // Intermediate texture for global filter chain
    wgpu::Texture globalIntermediateTexture_;
    wgpu::TextureView globalIntermediateView_;

    wgpu::Color clearColor_ = {0.0, 0.0, 0.0, 1.0};

    // Input state for interactive effects
    InputState inputState_;

    void createIntermediateTextures();
    void sortRenderablesByLayer();
};
