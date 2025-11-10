#pragma once

#include "../utils/Common.h"
#include "../utils/Vec2.h"
#include "../utils/Vec3.h"
#include "../utils/Vec4.h"
#include <string>
#include <map>
#include <variant>
#include <memory>

namespace clipengine {

/**
 * @brief Parameter value type
 * Supports float, int, bool, vec2, vec3, vec4
 */
using EffectParameterValue = std::variant<float, int, bool, Vec2, Vec3, Vec4>;

/**
 * @brief Effect parameter descriptor
 */
struct EffectParameter {
    std::string name;
    EffectParameterValue value;
    EffectParameterValue defaultValue;
    EffectParameterValue minValue;
    EffectParameterValue maxValue;

    EffectParameter() = default;

    EffectParameter(const std::string& n, float val, float min = 0.0f, float max = 1.0f)
        : name(n), value(val), defaultValue(val), minValue(min), maxValue(max) {}

    EffectParameter(const std::string& n, int val, int min = 0, int max = 100)
        : name(n), value(val), defaultValue(val), minValue(min), maxValue(max) {}

    EffectParameter(const std::string& n, bool val)
        : name(n), value(val), defaultValue(val), minValue(false), maxValue(true) {}
};

/**
 * @brief Base class for all effects
 *
 * Effect can be applied to a Layer to modify its appearance.
 * Each effect has adjustable parameters.
 *
 * Example effects:
 * - ColorAdjust (brightness, contrast, saturation)
 * - Blur (radius, quality)
 * - Glow (intensity, threshold)
 *
 * Example usage:
 * @code
 * auto colorAdjust = std::make_shared<ColorAdjustEffect>();
 * colorAdjust->setParameter("brightness", 1.2f);
 * colorAdjust->setParameter("contrast", 1.1f);
 * layer->addEffect(colorAdjust);
 * @endcode
 */
class Effect {
public:
    virtual ~Effect() = default;

    /**
     * @brief Initialize the effect with WebGPU device
     */
    virtual bool initialize(wgpu::Device device, wgpu::TextureFormat format) = 0;

    /**
     * @brief Apply effect to input texture
     * @param inputTexture Input texture view
     * @param outputTexture Output texture view (render target)
     * @param time Current time in seconds
     */
    virtual void apply(wgpu::TextureView inputTexture, wgpu::TextureView outputTexture, float time) = 0;

    /**
     * @brief Get effect name
     */
    virtual const std::string& getName() const = 0;

    // ========================================================================
    // Parameter Management
    // ========================================================================

    /**
     * @brief Set parameter value (float)
     */
    void setParameter(const std::string& name, float value) {
        if (parameters_.count(name)) {
            parameters_[name].value = value;
            parametersDirty_ = true;
        }
    }

    /**
     * @brief Set parameter value (int)
     */
    void setParameter(const std::string& name, int value) {
        if (parameters_.count(name)) {
            parameters_[name].value = value;
            parametersDirty_ = true;
        }
    }

    /**
     * @brief Set parameter value (bool)
     */
    void setParameter(const std::string& name, bool value) {
        if (parameters_.count(name)) {
            parameters_[name].value = value;
            parametersDirty_ = true;
        }
    }

    /**
     * @brief Set parameter value (vec2)
     */
    void setParameter(const std::string& name, const Vec2& value) {
        if (parameters_.count(name)) {
            parameters_[name].value = value;
            parametersDirty_ = true;
        }
    }

    /**
     * @brief Set parameter value (vec3)
     */
    void setParameter(const std::string& name, const Vec3& value) {
        if (parameters_.count(name)) {
            parameters_[name].value = value;
            parametersDirty_ = true;
        }
    }

    /**
     * @brief Set parameter value (vec4)
     */
    void setParameter(const std::string& name, const Vec4& value) {
        if (parameters_.count(name)) {
            parameters_[name].value = value;
            parametersDirty_ = true;
        }
    }

    /**
     * @brief Get parameter value
     */
    template<typename T>
    T getParameter(const std::string& name, const T& defaultValue = T()) const {
        auto it = parameters_.find(name);
        if (it != parameters_.end()) {
            if (std::holds_alternative<T>(it->second.value)) {
                return std::get<T>(it->second.value);
            }
        }
        return defaultValue;
    }

    /**
     * @brief Check if effect has parameter
     */
    bool hasParameter(const std::string& name) const {
        return parameters_.count(name) > 0;
    }

    /**
     * @brief Get all parameters
     */
    const std::map<std::string, EffectParameter>& getParameters() const {
        return parameters_;
    }

    /**
     * @brief Reset parameter to default value
     */
    void resetParameter(const std::string& name) {
        if (parameters_.count(name)) {
            parameters_[name].value = parameters_[name].defaultValue;
            parametersDirty_ = true;
        }
    }

    /**
     * @brief Reset all parameters to default values
     */
    void resetAllParameters() {
        for (auto& [name, param] : parameters_) {
            param.value = param.defaultValue;
        }
        parametersDirty_ = true;
    }

    // ========================================================================
    // Enable/Disable
    // ========================================================================

    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

protected:
    wgpu::Device device_;
    wgpu::TextureFormat format_;

    bool enabled_ = true;
    bool parametersDirty_ = true;

    // Parameters storage
    std::map<std::string, EffectParameter> parameters_;

    /**
     * @brief Register a parameter
     * Call this in derived class constructor
     */
    void registerParameter(const EffectParameter& param) {
        parameters_[param.name] = param;
    }

    /**
     * @brief Update uniform buffer with current parameter values
     * Call this when parametersDirty_ is true
     */
    virtual void updateParameterBuffer() = 0;
};

/**
 * @brief Helper macros for common effects
 */
#define EFFECT_PARAMETER_FLOAT(name, defaultVal, minVal, maxVal) \
    registerParameter(EffectParameter(name, defaultVal, minVal, maxVal))

#define EFFECT_PARAMETER_INT(name, defaultVal, minVal, maxVal) \
    registerParameter(EffectParameter(name, defaultVal, minVal, maxVal))

#define EFFECT_PARAMETER_BOOL(name, defaultVal) \
    registerParameter(EffectParameter(name, defaultVal))

} // namespace clipengine
