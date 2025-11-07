#pragma once
#include "../utils/Common.h"
#include <string>

/**
 * @brief Layer type in the composition
 *
 * Defines the type of content this layer renders.
 * Matches concepts from professional video editing software (After Effects, Premiere Pro).
 */
enum class LayerType {
    Video,      ///< Video layer (moving image sequence)
    Image,      ///< Image layer (static image)
    Text,       ///< Text layer (typography)
    Shape,      ///< Shape layer (vector graphics)
    Audio,      ///< Audio layer (sound)
    Effect,     ///< Effect layer (shader effects)
    Adjustment  ///< Adjustment layer (affects layers below)
};

/**
 * @brief Layer viewport/transform
 *
 * Defines position and size of the layer in normalized coordinates [0, 1].
 * Similar to layer transform in After Effects/Premiere Pro.
 */
struct LayerTransform {
    float x = 0.0f;      ///< X position (0 = left, 1 = right)
    float y = 0.0f;      ///< Y position (0 = top, 1 = bottom)
    float width = 1.0f;  ///< Width (0-1, relative to canvas)
    float height = 1.0f; ///< Height (0-1, relative to canvas)
};

/**
 * @brief Base class for all composition layers
 *
 * CompositionLayer is the base interface for all renderable elements in a composition.
 * Each layer represents a single visual element (video, image, text, effect, etc.)
 * that can be composited together.
 *
 * Layer Hierarchy Concept (like After Effects/Premiere Pro):
 * - Each layer has a z-order (layer number) that determines stacking order
 * - Lower numbers render first (background), higher numbers render last (foreground)
 * - Each layer can be enabled/disabled (like the eye icon in AE/Premiere)
 * - Each layer has a name for identification
 * - Each layer has a transform (position, size) within the composition
 *
 * Example usage:
 * @code
 * // Create a video layer
 * auto videoLayer = std::make_unique<VideoRenderer>();
 * videoLayer->setLayer(0);           // Background layer
 * videoLayer->setName("Background Video");
 * videoLayer->setTransform(0, 0, 1, 1);  // Full screen
 *
 * // Create a PiP (Picture-in-Picture) layer
 * auto pipLayer = std::make_unique<VideoRenderer>();
 * pipLayer->setLayer(1);             // On top of background
 * pipLayer->setName("PiP Overlay");
 * pipLayer->setTransform(0.7f, 0.1f, 0.25f, 0.25f);  // Small corner
 * @endcode
 */
class CompositionLayer {
public:
    virtual ~CompositionLayer() = default;

    /**
     * @brief Initialize the layer with WebGPU device
     */
    virtual bool initialize(wgpu::Device device, wgpu::TextureFormat format) = 0;

    /**
     * @brief Render the layer to the current render pass
     */
    virtual void render(wgpu::RenderPassEncoder& pass) = 0;

    /**
     * @brief Update the layer (animations, time-based effects)
     * @param deltaTime Time since last frame in seconds
     */
    virtual void update(float deltaTime) = 0;

    /**
     * @brief Get the type of this layer
     */
    virtual LayerType getType() const = 0;

    /**
     * @brief Set layer transform (position and size)
     */
    virtual void setTransform(float x, float y, float width, float height) = 0;

    // ========================================================================
    // Layer Visibility (Eye Icon in AE/Premiere)
    // ========================================================================

    /**
     * @brief Enable or disable layer rendering
     * Like the "eye" icon in After Effects/Premiere Pro
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief Check if layer is enabled
     */
    bool isEnabled() const { return enabled_; }

    // ========================================================================
    // Layer Order (Z-Index / Stacking Order)
    // ========================================================================

    /**
     * @brief Set layer z-order (stacking order)
     * Lower numbers render first (background), higher numbers on top (foreground)
     * @param layer Layer number (0 = bottom, higher = on top)
     */
    void setLayer(int layer) { layer_ = layer; }

    /**
     * @brief Get layer z-order
     */
    int getLayer() const { return layer_; }

    // ========================================================================
    // Layer Name (Like Timeline Name in AE/Premiere)
    // ========================================================================

    /**
     * @brief Set layer name (for identification in timeline)
     */
    void setName(const std::string& name) { name_ = name; }

    /**
     * @brief Set layer name (for identification in timeline)
     */
    void setName(const char* name) { name_ = name ? name : ""; }

    /**
     * @brief Get layer name
     */
    const std::string& getName() const { return name_; }

    // ========================================================================
    // Layer Transform (Position, Size)
    // ========================================================================

    /**
     * @brief Get layer transform
     */
    const LayerTransform& getTransform() const { return transform_; }

protected:
    wgpu::Device device_;
    wgpu::TextureFormat surfaceFormat_;

    bool enabled_ = true;    ///< Layer visibility (eye icon)
    int layer_ = 0;          ///< Layer order (z-index)
    std::string name_;       ///< Layer name (timeline label)
    LayerTransform transform_; ///< Layer position and size
};

/**
 * @brief Factory for creating composition layers
 */
class CompositionLayerFactory {
public:
    /**
     * @brief Create a video layer
     */
    static std::unique_ptr<CompositionLayer> createVideoLayer();

    // Future layer types:
    // static std::unique_ptr<CompositionLayer> createImageLayer();
    // static std::unique_ptr<CompositionLayer> createTextLayer();
    // static std::unique_ptr<CompositionLayer> createShapeLayer();
};
