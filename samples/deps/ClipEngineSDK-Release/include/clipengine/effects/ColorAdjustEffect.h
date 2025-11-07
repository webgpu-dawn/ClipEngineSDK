#pragma once

#include "Effect.h"

namespace clipengine {

/**
 * @brief Color adjustment effect
 *
 * Provides adjustable parameters for:
 * - Brightness
 * - Contrast
 * - Saturation
 * - Exposure
 * - Gain
 *
 * Example usage:
 * @code
 * auto colorAdjust = std::make_shared<ColorAdjustEffect>();
 * colorAdjust->setParameter("brightness", 1.2f);  // 20% brighter
 * colorAdjust->setParameter("contrast", 1.1f);     // 10% more contrast
 * colorAdjust->setParameter("saturation", 0.8f);   // 20% less saturated
 * layer->addEffect(colorAdjust);
 * @endcode
 */
class ColorAdjustEffect : public Effect {
public:
    ColorAdjustEffect();
    ~ColorAdjustEffect() override = default;

    bool initialize(wgpu::Device device, wgpu::TextureFormat format) override;
    void apply(wgpu::TextureView inputTexture, wgpu::TextureView outputTexture, float time) override;
    const std::string& getName() const override { return name_; }

    // ========================================================================
    // Convenient setters (type-safe)
    // ========================================================================

    /**
     * @brief Set brightness (1.0 = original, >1.0 = brighter, <1.0 = darker)
     */
    void setBrightness(float value) {
        setParameter("brightness", value);
    }

    float getBrightness() const {
        return getParameter<float>("brightness", 1.0f);
    }

    /**
     * @brief Set contrast (1.0 = original, >1.0 = more contrast, <1.0 = less)
     */
    void setContrast(float value) {
        setParameter("contrast", value);
    }

    float getContrast() const {
        return getParameter<float>("contrast", 1.0f);
    }

    /**
     * @brief Set saturation (1.0 = original, 0.0 = grayscale, >1.0 = oversaturated)
     */
    void setSaturation(float value) {
        setParameter("saturation", value);
    }

    float getSaturation() const {
        return getParameter<float>("saturation", 1.0f);
    }

    /**
     * @brief Set exposure (-1.0 to 1.0, 0.0 = original)
     */
    void setExposure(float value) {
        setParameter("exposure", value);
    }

    float getExposure() const {
        return getParameter<float>("exposure", 0.0f);
    }

    /**
     * @brief Set gain (1.0 = original, >1.0 = amplify, <1.0 = reduce)
     */
    void setGain(float value) {
        setParameter("gain", value);
    }

    float getGain() const {
        return getParameter<float>("gain", 1.0f);
    }

protected:
    void updateParameterBuffer() override;

private:
    void createPipeline();
    void createBindGroup(wgpu::TextureView inputTexture, wgpu::TextureView outputTexture);

    std::string name_ = "ColorAdjust";

    // GPU resources
    wgpu::ShaderModule shaderModule_;
    wgpu::RenderPipeline pipeline_;
    wgpu::BindGroup bindGroup_;
    wgpu::BindGroupLayout bindGroupLayout_;
    wgpu::Buffer uniformBuffer_;
    wgpu::Sampler sampler_;
    wgpu::Buffer vertexBuffer_;

    // Uniform data structure (must match shader)
    struct ColorAdjustUniforms {
        float brightness = 1.0f;
        float contrast = 1.0f;
        float saturation = 1.0f;
        float exposure = 0.0f;
        float gain = 1.0f;
        float padding[3];  // Align to 16 bytes
    };
};

} // namespace clipengine
