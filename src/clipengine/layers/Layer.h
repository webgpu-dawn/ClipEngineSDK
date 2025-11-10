#pragma once

#include "../utils/Common.h"
#include "../transform/Transform2D.h"
#include <string>
#include <vector>
#include <memory>

namespace clipengine {

// Forward declarations
class Effect;

/**
 * @brief Layer type enumeration
 */
enum class LayerType {
    Video,      ///< Video layer (moving image sequence)
    Image,      ///< Image layer (static image)
    Solid,      ///< Solid color layer
    Text,       ///< Text layer (typography)
    Shape,      ///< Shape layer (vector graphics)
    Adjustment  ///< Adjustment layer (affects layers below)
};

/**
 * @brief Blend mode for layer compositing
 */
enum class BlendMode {
    Normal,      ///< Normal alpha blending
    Add,         ///< Additive blending
    Multiply,    ///< Multiply blending
    Screen,      ///< Screen blending
    Overlay      ///< Overlay blending
};

/**
 * @brief Base class for all layers
 *
 * Layer represents a single visual element in a composition.
 * Each layer can have:
 * - Transform (position, rotation, scale)
 * - Effects (color correction, blur, etc.) with adjustable parameters
 * - Time range (start time, duration)
 * - Visibility and opacity
 * - Blend mode
 *
 * Design Philosophy:
 * - Layer → Transform → Effect Chain → Output
 * - Each layer renders to a GPU texture
 * - Layers are composited in z-order by the Compositor
 *
 * Example usage:
 * @code
 * auto videoLayer = std::make_shared<VideoLayer>();
 * videoLayer->loadVideo("input.mp4");
 * videoLayer->transform.position = {100.0f, 200.0f};
 * videoLayer->transform.rotation = 45.0f;
 *
 * // Add effect with adjustable parameters
 * auto colorAdjust = std::make_shared<ColorAdjustEffect>();
 * colorAdjust->setParameter("brightness", 1.2f);
 * colorAdjust->setParameter("contrast", 1.1f);
 * videoLayer->addEffect(colorAdjust);
 * @endcode
 */
class Layer {
public:
    virtual ~Layer() = default;

    // ========================================================================
    // Core Layer Interface
    // ========================================================================

    /**
     * @brief Initialize the layer with WebGPU device
     * @param device WebGPU device
     * @param format Output texture format
     * @return true if initialization succeeded
     */
    virtual bool initialize(wgpu::Device device, wgpu::TextureFormat format) = 0;

    /**
     * @brief Render the layer to a texture
     * @param time Current time in seconds (for time-based effects)
     * @return TextureView containing the rendered result
     */
    virtual wgpu::TextureView render(float time) = 0;

    /**
     * @brief Update the layer (called every frame before render)
     * @param deltaTime Time since last frame in seconds
     */
    virtual void update(float deltaTime) = 0;

    /**
     * @brief Get the layer type
     */
    virtual LayerType getType() const = 0;

    /**
     * @brief Get the layer's native size in pixels
     * @return Size of the layer content (before transform)
     */
    virtual Vec2 getSize() const = 0;

    // ========================================================================
    // Layer Properties
    // ========================================================================

    /**
     * @brief Set layer name (for identification)
     */
    void setName(const std::string& name) { name_ = name; }
    const std::string& getName() const { return name_; }

    /**
     * @brief Enable/disable layer visibility
     */
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

    /**
     * @brief Set layer opacity (0.0 = transparent, 1.0 = opaque)
     */
    void setOpacity(float opacity) {
        transform.opacity = (opacity < 0.0f) ? 0.0f : (opacity > 1.0f) ? 1.0f : opacity;
    }
    float getOpacity() const { return transform.opacity; }

    /**
     * @brief Set blend mode for compositing
     */
    void setBlendMode(BlendMode mode) { blendMode_ = mode; }
    BlendMode getBlendMode() const { return blendMode_; }

    /**
     * @brief Set layer z-order (stacking order)
     * Lower values render first (background), higher values on top (foreground)
     */
    void setZOrder(int order) { zOrder_ = order; }
    int getZOrder() const { return zOrder_; }

    // ========================================================================
    // Time Range
    // ========================================================================

    /**
     * @brief Set time range for this layer
     * @param startTime When the layer starts (seconds)
     * @param duration How long the layer is visible (seconds)
     */
    void setTimeRange(float startTime, float duration) {
        startTime_ = startTime;
        duration_ = duration;
    }

    float getStartTime() const { return startTime_; }
    float getDuration() const { return duration_; }

    /**
     * @brief Check if layer is active at given time
     */
    bool isActiveAtTime(float time) const {
        return time >= startTime_ && time < (startTime_ + duration_);
    }

    // ========================================================================
    // Effect Chain
    // ========================================================================

    /**
     * @brief Add an effect to this layer
     * Effects are applied in the order they are added
     */
    void addEffect(std::shared_ptr<Effect> effect) {
        effects_.push_back(effect);
    }

    /**
     * @brief Remove an effect by index
     */
    void removeEffect(size_t index) {
        if (index < effects_.size()) {
            effects_.erase(effects_.begin() + index);
        }
    }

    /**
     * @brief Clear all effects
     */
    void clearEffects() {
        effects_.clear();
    }

    /**
     * @brief Get all effects
     */
    const std::vector<std::shared_ptr<Effect>>& getEffects() const {
        return effects_;
    }

    /**
     * @brief Get effect by index
     */
    std::shared_ptr<Effect> getEffect(size_t index) const {
        if (index < effects_.size()) {
            return effects_[index];
        }
        return nullptr;
    }

    /**
     * @brief Get number of effects
     */
    size_t getEffectCount() const {
        return effects_.size();
    }

    // ========================================================================
    // Public Members
    // ========================================================================

    Transform2D transform;  ///< Layer transformation (position, rotation, scale, etc.)

protected:
    // WebGPU resources
    wgpu::Device device_;
    wgpu::TextureFormat surfaceFormat_;

    // Layer properties
    std::string name_;
    bool visible_ = true;
    BlendMode blendMode_ = BlendMode::Normal;
    int zOrder_ = 0;

    // Time range
    float startTime_ = 0.0f;
    float duration_ = std::numeric_limits<float>::max();  // Default: infinite duration

    // Effect chain
    std::vector<std::shared_ptr<Effect>> effects_;

    /**
     * @brief Apply effects to input texture
     * @param input Input texture view
     * @param time Current time
     * @return Texture view with effects applied
     */
    wgpu::TextureView applyEffects(wgpu::TextureView input, float time);
};

} // namespace clipengine
