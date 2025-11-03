#pragma once

#include "../common/Common.h"
#include "../core/CompositionLayer.h"
#include "FilterChain.h"
#include "InputState.h"
#include <vector>
#include <memory>
#include <algorithm>

/**
 * @brief Multi-layer composition engine for video editing
 *
 * CompositionEngine manages multiple layers (videos, images, text, animations,
 * effects) and composites them into a final output. Each layer is a
 * CompositionLayer object with its own z-order and can have its own filter chain.
 *
 * Layer Concept (like After Effects / Premiere Pro):
 * - Each CompositionLayer represents a single layer in the timeline
 * - Layers are sorted by z-order (layer number) before rendering
 * - Lower layer numbers render first (background), higher numbers on top (foreground)
 * - Each layer can be enabled/disabled (like the "eye" icon in AE/Premiere)
 * - Each layer has a name (shown in timeline)
 * - Each layer has a transform (position, size)
 * - Supported layer types:
 *   • Video layers (VideoRenderer)
 *   • Image layers (ImageRenderer - future)
 *   • Text layers (TextRenderer - future)
 *   • Shape layers (ShapeRenderer - future)
 *   • Effect layers (custom shaders)
 *
 * Features:
 * - Multiple layers with automatic z-ordering
 * - Per-layer filter chains (individual effects)
 * - Global post-processing filters (applied to final output)
 * - Interactive input support (mouse, keyboard)
 * - Efficient rendering with minimal texture copies
 *
 * Example usage:
 * @code
 * CompositionEngine engine;
 * engine.initialize(device, format, 1920, 1080);
 *
 * // Add background video (layer 0)
 * auto bgVideo = std::make_unique<VideoRenderer>();
 * bgVideo->setLayer(0);
 * bgVideo->setName("Background Video");
 * size_t bgIndex = engine.addLayer(std::move(bgVideo));
 *
 * // Add overlay video (layer 1) - Picture-in-Picture
 * auto pipVideo = std::make_unique<VideoRenderer>();
 * pipVideo->setLayer(1);
 * pipVideo->setName("PiP Overlay");
 * pipVideo->setTransform(0.7f, 0.1f, 0.25f, 0.25f);  // Small corner
 * size_t pipIndex = engine.addLayer(std::move(pipVideo));
 *
 * // Add text layer (layer 2) - future
 * // auto textLayer = std::make_unique<TextRenderer>();
 * // textLayer->setLayer(2);
 * // textLayer->setName("Title Text");
 * // engine.addLayer(std::move(textLayer));
 *
 * // Add per-layer effect (like layer effects in AE)
 * engine.getLayerFilterChain(bgIndex)->addEffect(...);
 *
 * // Add global adjustment layer effect (affects all layers below)
 * engine.getGlobalFilterChain().addEffect(...);
 *
 * // Render frame
 * engine.render(outputView);
 * @endcode
 */
class CompositionEngine {
public:
    CompositionEngine() = default;
    ~CompositionEngine() = default;

    /**
     * @brief Initialize the composition engine
     * @param device WebGPU device
     * @param format Output texture format
     * @param width Output width in pixels
     * @param height Output height in pixels
     */
    bool initialize(wgpu::Device device, wgpu::TextureFormat format, uint32_t width, uint32_t height);

    /**
     * @brief Add a layer to the composition
     * @param layer Unique pointer to layer (ownership transferred)
     * @return Index of the added layer
     */
    size_t addLayer(std::unique_ptr<CompositionLayer> layer);

    /**
     * @brief Remove a layer by index
     */
    void removeLayer(size_t index);

    /**
     * @brief Get layer at index
     */
    CompositionLayer* getLayer(size_t index);

    /**
     * @brief Get number of layers in composition
     */
    size_t getLayerCount() const { return layers_.size(); }

    /**
     * @brief Clear all layers from composition
     */
    void clearLayers();

    /**
     * @brief Render all layers to the given texture view
     * @param outputView Target texture view
     */
    void render(wgpu::TextureView outputView);

    /**
     * @brief Update all layers (animations, transitions, etc.)
     * @param deltaTime Time delta in seconds
     */
    void update(float deltaTime);

    /**
     * @brief Resize the composition output
     */
    void resize(uint32_t width, uint32_t height);

    /**
     * @brief Get global post-processing filter chain
     * Like an "Adjustment Layer" in After Effects - affects all layers below
     */
    FilterChain& getGlobalFilterChain() { return globalFilterChain_; }

    /**
     * @brief Get filter chain for a specific layer
     * Like "Layer Effects" in After Effects/Premiere Pro
     * @param index Layer index
     * @return Pointer to filter chain, or nullptr if invalid index
     */
    FilterChain* getLayerFilterChain(size_t index);

    /**
     * @brief Set clear color (background color)
     */
    void setClearColor(float r, float g, float b, float a) {
        clearColor_ = {r, g, b, a};
    }

    /**
     * @brief Get input state for reading/writing
     * Used for interactive effects that respond to user input
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
    struct LayerEntry {
        std::unique_ptr<CompositionLayer> layer;
        FilterChain filterChain;
        wgpu::Texture intermediateTexture;
        wgpu::TextureView intermediateView;
        bool needsIntermediateTexture = false;

        // Allow move construction and assignment
        LayerEntry() = default;
        LayerEntry(LayerEntry&&) = default;
        LayerEntry& operator=(LayerEntry&&) = default;
        // Disable copy
        LayerEntry(const LayerEntry&) = delete;
        LayerEntry& operator=(const LayerEntry&) = delete;
    };

    wgpu::Device device_;
    wgpu::TextureFormat format_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    std::vector<LayerEntry> layers_;
    FilterChain globalFilterChain_;

    // Intermediate texture for global filter chain
    wgpu::Texture globalIntermediateTexture_;
    wgpu::TextureView globalIntermediateView_;

    wgpu::Color clearColor_ = {0.0, 0.0, 0.0, 1.0};

    // Input state for interactive effects
    InputState inputState_;

    void createIntermediateTextures();
    void sortLayersByOrder();
};
