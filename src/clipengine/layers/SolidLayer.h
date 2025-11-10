#pragma once

#include "Layer.h"
#include "../utils/Vec4.h"

namespace clipengine {

/**
 * @brief Solid color layer
 *
 * SolidLayer renders a solid color rectangle.
 * Useful for backgrounds, overlays, and color fills.
 *
 * Example usage:
 * @code
 * // Create red background
 * auto background = std::make_shared<SolidLayer>();
 * background->setColor({1.0f, 0.0f, 0.0f, 1.0f});  // Red
 * background->setSize(1920, 1080);
 *
 * // Create semi-transparent overlay
 * auto overlay = std::make_shared<SolidLayer>();
 * overlay->setColor({0.0f, 0.0f, 0.0f, 0.5f});  // 50% black
 * @endcode
 */
class SolidLayer : public Layer {
public:
    /**
     * @brief Construct with default black color
     */
    SolidLayer();

    /**
     * @brief Construct with specific color
     * @param color RGBA color (0.0-1.0 range)
     */
    explicit SolidLayer(const Vec4& color);

    /**
     * @brief Construct with specific color and size
     * @param color RGBA color
     * @param width Layer width in pixels
     * @param height Layer height in pixels
     */
    SolidLayer(const Vec4& color, uint32_t width, uint32_t height);

    ~SolidLayer() override;

    // ========================================================================
    // Layer Interface Implementation
    // ========================================================================

    bool initialize(wgpu::Device device, wgpu::TextureFormat format) override;
    wgpu::TextureView render(float time) override;
    void update(float deltaTime) override;
    LayerType getType() const override { return LayerType::Solid; }
    Vec2 getSize() const override;

    // ========================================================================
    // Solid Layer Properties
    // ========================================================================

    /**
     * @brief Set solid color
     * @param color RGBA color (0.0-1.0 range)
     */
    void setColor(const Vec4& color);

    /**
     * @brief Set solid color with RGB values
     * @param r Red (0.0-1.0)
     * @param g Green (0.0-1.0)
     * @param b Blue (0.0-1.0)
     * @param a Alpha (0.0-1.0)
     */
    void setColor(float r, float g, float b, float a = 1.0f);

    /**
     * @brief Get current color
     */
    const Vec4& getColor() const { return color_; }

    /**
     * @brief Set layer size
     * @param width Width in pixels
     * @param height Height in pixels
     */
    void setSize(uint32_t width, uint32_t height);

    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }

private:
    void createPipeline();
    void createTexture();
    void updateTexture();

    Vec4 color_ = {0.0f, 0.0f, 0.0f, 1.0f};  // Default black
    uint32_t width_ = 1920;
    uint32_t height_ = 1080;
    bool colorDirty_ = true;

    // GPU resources
    wgpu::Texture texture_;
    wgpu::TextureView textureView_;
    wgpu::Buffer vertexBuffer_;
    wgpu::Buffer uniformBuffer_;
    wgpu::RenderPipeline pipeline_;
};

} // namespace clipengine
