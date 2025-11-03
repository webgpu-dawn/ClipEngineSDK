#pragma once

#include "Filter.h"

/**
 * @brief Color adjustment filter with brightness, contrast, saturation, and hue
 *
 * This filter allows real-time adjustment of color properties.
 * All parameters can be modified at runtime.
 */
class ColorAdjustFilter : public ShaderFilter {
public:
    struct Parameters {
        float brightness = 0.0f;  // Range: -1.0 to 1.0
        float contrast = 1.0f;    // Range: 0.0 to 2.0
        float saturation = 1.0f;  // Range: 0.0 to 2.0
        float hue = 0.0f;         // Range: -180 to 180 degrees (stored as radians)
    };

    ColorAdjustFilter();
    ~ColorAdjustFilter() override = default;

    void updateParameters() override;

    // Parameter setters
    void setBrightness(float value);  // -1.0 to 1.0
    void setContrast(float value);    // 0.0 to 2.0
    void setSaturation(float value);  // 0.0 to 2.0
    void setHue(float degrees);       // -180 to 180

    // Get current parameters
    const Parameters& getParameters() const { return params_; }

private:
    Parameters params_;
    bool paramsDirty_ = true;

    static ShaderConfig createShaderConfig();
};
