#pragma once

#include "Filter.h"
#include <array>
#include <map>
#include <string>
#include <variant>
#include <vector>

/**
 * @brief Shader parameter value types
 */
using ShaderParamValue = std::variant<float, int, bool, std::array<float, 2>, std::array<float, 3>, std::array<float, 4>>;

/**
 * @brief Parameter descriptor for shader effects
 */
struct ShaderParam {
    std::string name;
    ShaderParamValue defaultValue;
    ShaderParamValue minValue;
    ShaderParamValue maxValue;
    std::string description;
};

/**
 * @brief Generic shader-based effect/filter
 *
 * This is a universal filter class that can represent ANY shader-based effect.
 * Instead of creating a new C++ class for each effect, you simply provide:
 * 1. Shader code (vertex + fragment)
 * 2. Parameter definitions
 * 3. Set parameter values at runtime
 *
 * Example:
 *   auto blur = ShaderEffect::createBlur(radius);
 *   auto colorAdjust = ShaderEffect::createColorAdjust();
 *   colorAdjust->setParam("brightness", 0.2f);
 *   colorAdjust->setParam("contrast", 1.3f);
 */
class ShaderEffect : public ShaderFilter {
public:
    /**
     * @brief Create a shader effect from shader code and parameters
     * @param name Effect name
     * @param config Shader configuration
     * @param params Parameter definitions
     */
    ShaderEffect(const std::string& name,
                 const ShaderConfig& config,
                 const std::vector<ShaderParam>& params = {});

    ~ShaderEffect() override = default;

    /**
     * @brief Set a parameter value
     * @param name Parameter name
     * @param value Parameter value
     * @return true if parameter exists and was set
     */
    bool setParam(const std::string& name, const ShaderParamValue& value);

    /**
     * @brief Get a parameter value
     */
    ShaderParamValue getParam(const std::string& name) const;

    /**
     * @brief Get all parameter names
     */
    std::vector<std::string> getParamNames() const;

    /**
     * @brief Get parameter descriptor
     */
    const ShaderParam* getParamDescriptor(const std::string& name) const;

    /**
     * @brief Enable automatic input binding
     * When enabled, the effect will automatically read values from InputState
     * and bind them to shader uniforms with matching names.
     *
     * Supported input variable names:
     * - "iMouse" -> vec4(mouse.x, mouse.y, clickX, clickY)
     * - "iTime" -> elapsed time
     * - "iTimeDelta" -> delta time
     * - "iFrame" -> frame count
     * - "iResolution" -> vec3(width, height, aspectRatio)
     */
    void enableInputBinding(bool enable) { useInputBinding_ = enable; }
    bool isInputBindingEnabled() const { return useInputBinding_; }

    void updateParameters() override;

    // ========================================================================
    // Factory methods for common effects
    // ========================================================================

    /**
     * @brief Create a brightness/contrast/saturation/hue adjustment effect
     */
    static std::unique_ptr<ShaderEffect> createColorAdjust();

    /**
     * @brief Create a Gaussian blur effect
     * @param radius Blur radius (default: 5.0)
     */
    static std::unique_ptr<ShaderEffect> createBlur(float radius = 5.0f);

    /**
     * @brief Create a sharpen effect
     * @param amount Sharpen amount (default: 1.0)
     */
    static std::unique_ptr<ShaderEffect> createSharpen(float amount = 1.0f);

    /**
     * @brief Create a vignette effect
     */
    static std::unique_ptr<ShaderEffect> createVignette();

    /**
     * @brief Create a chromatic aberration effect
     */
    static std::unique_ptr<ShaderEffect> createChromaticAberration();

    /**
     * @brief Create a mouse spotlight effect (interactive)
     * Requires input binding to be enabled
     */
    static std::unique_ptr<ShaderEffect> createMouseSpotlight();

    /**
     * @brief Create a pixelation effect with time animation (interactive)
     * Requires input binding to be enabled
     */
    static std::unique_ptr<ShaderEffect> createPixelation();

private:
    std::map<std::string, ShaderParam> paramDescriptors_;
    std::map<std::string, ShaderParamValue> paramValues_;
    bool paramsDirty_ = true;
    bool useInputBinding_ = false;

    void packUniformData(std::vector<float>& buffer);
    void packInputData(std::vector<float>& buffer);
};

/**
 * @brief Shader effect presets library
 *
 * This namespace contains factory functions to create common shader effects
 * without needing to write shader code manually.
 */
namespace ShaderEffectPresets {
    /**
     * @brief Create shader config for color adjustment
     */
    ShaderConfig createColorAdjustConfig();

    /**
     * @brief Create shader config for Gaussian blur
     */
    ShaderConfig createBlurConfig();

    /**
     * @brief Create shader config for sharpen
     */
    ShaderConfig createSharpenConfig();

    /**
     * @brief Create shader config for vignette
     */
    ShaderConfig createVignetteConfig();

    /**
     * @brief Create shader config for chromatic aberration
     */
    ShaderConfig createChromaticAberrationConfig();

    /**
     * @brief Create shader config for mouse spotlight effect
     */
    ShaderConfig createMouseSpotlightConfig();

    /**
     * @brief Create shader config for pixelation effect
     */
    ShaderConfig createPixelationConfig();
}
