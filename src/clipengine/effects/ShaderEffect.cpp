#include "ShaderEffect.h"
#include "EffectLoader.h"
#include <algorithm>
#include <cstring>

ShaderEffect::ShaderEffect(const std::string& name,
                           const ShaderConfig& config,
                           const std::vector<ShaderParam>& params)
    : ShaderFilter(name, config) {

    // Register parameters
    for (const auto& param : params) {
        paramDescriptors_[param.name] = param;
        paramValues_[param.name] = param.defaultValue;
    }
}

bool ShaderEffect::setParam(const std::string& name, const ShaderParamValue& value) {
    auto it = paramDescriptors_.find(name);
    if (it == paramDescriptors_.end()) {
        return false; // Parameter not found
    }

    // TODO: Add value range clamping based on min/max

    paramValues_[name] = value;
    paramsDirty_ = true;
    return true;
}

ShaderParamValue ShaderEffect::getParam(const std::string& name) const {
    auto it = paramValues_.find(name);
    if (it != paramValues_.end()) {
        return it->second;
    }
    // Return default value if not found
    auto descIt = paramDescriptors_.find(name);
    if (descIt != paramDescriptors_.end()) {
        return descIt->second.defaultValue;
    }
    return 0.0f; // Fallback
}

std::vector<std::string> ShaderEffect::getParamNames() const {
    std::vector<std::string> names;
    names.reserve(paramDescriptors_.size());
    for (const auto& [name, _] : paramDescriptors_) {
        names.push_back(name);
    }
    return names;
}

const ShaderParam* ShaderEffect::getParamDescriptor(const std::string& name) const {
    auto it = paramDescriptors_.find(name);
    return (it != paramDescriptors_.end()) ? &it->second : nullptr;
}

void ShaderEffect::packUniformData(std::vector<float>& buffer) {
    buffer.clear();

    // Pack parameters in alphabetical order for consistency
    std::vector<std::string> sortedNames;
    for (const auto& [name, _] : paramDescriptors_) {
        sortedNames.push_back(name);
    }
    std::sort(sortedNames.begin(), sortedNames.end());

    for (const auto& name : sortedNames) {
        const auto& value = paramValues_[name];

        std::visit([&buffer](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, float>) {
                buffer.push_back(arg);
            } else if constexpr (std::is_same_v<T, int>) {
                buffer.push_back(static_cast<float>(arg));
            } else if constexpr (std::is_same_v<T, bool>) {
                buffer.push_back(arg ? 1.0f : 0.0f);
            } else if constexpr (std::is_same_v<T, std::array<float, 2>>) {
                buffer.push_back(arg[0]);
                buffer.push_back(arg[1]);
            } else if constexpr (std::is_same_v<T, std::array<float, 3>>) {
                buffer.push_back(arg[0]);
                buffer.push_back(arg[1]);
                buffer.push_back(arg[2]);
            } else if constexpr (std::is_same_v<T, std::array<float, 4>>) {
                buffer.push_back(arg[0]);
                buffer.push_back(arg[1]);
                buffer.push_back(arg[2]);
                buffer.push_back(arg[3]);
            }
        }, value);
    }
}

void ShaderEffect::packInputData(std::vector<float>& buffer) {
    if (!inputState_) return;

    // Pack input data from InputState
    // Check which parameters are input parameters and pack them
    for (const auto& [name, _] : paramDescriptors_) {
        if (name == "iMouse") {
            buffer.push_back(inputState_->mouse.x);
            buffer.push_back(inputState_->mouse.y);
            buffer.push_back(inputState_->mouse.clickX);
            buffer.push_back(inputState_->mouse.clickY);
        } else if (name == "iTime") {
            buffer.push_back(inputState_->time.elapsed);
        } else if (name == "iTimeDelta") {
            buffer.push_back(inputState_->time.delta);
        } else if (name == "iFrame") {
            buffer.push_back(static_cast<float>(inputState_->time.frameCount));
        } else if (name == "iResolution") {
            buffer.push_back(static_cast<float>(inputState_->resolution.width));
            buffer.push_back(static_cast<float>(inputState_->resolution.height));
            buffer.push_back(inputState_->resolution.aspectRatio);
        }
    }
}

void ShaderEffect::updateParameters() {
    if (!paramsDirty_ && !useInputBinding_) return;

    std::vector<float> buffer;

    // Pack regular parameters
    if (paramsDirty_) {
        packUniformData(buffer);
    }

    // Pack input-bound parameters (always update if input binding is enabled)
    if (useInputBinding_ && inputState_) {
        packInputData(buffer);
    }

    if (!buffer.empty()) {
        updateUniformBuffer(buffer.data(), buffer.size() * sizeof(float));
    }

    paramsDirty_ = false;
}

// ============================================================================
// Factory Methods
// ============================================================================

std::unique_ptr<ShaderEffect> ShaderEffect::loadFromFile(const std::string& configPath) {
    return clipengine::EffectLoader::loadFromFile(configPath);
}

std::unique_ptr<ShaderEffect> ShaderEffect::createColorAdjust() {
    std::vector<ShaderParam> params = {
        {"brightness", 0.0f, -1.0f, 1.0f, "Brightness adjustment (-1 to 1)"},
        {"contrast", 1.0f, 0.0f, 2.0f, "Contrast adjustment (0 to 2)"},
        {"hue", 0.0f, -180.0f, 180.0f, "Hue shift in degrees (-180 to 180)"},
        {"saturation", 1.0f, 0.0f, 2.0f, "Saturation adjustment (0 to 2)"}
    };

    return std::make_unique<ShaderEffect>(
        "ColorAdjust",
        ShaderEffectPresets::createColorAdjustConfig(),
        params
    );
}

std::unique_ptr<ShaderEffect> ShaderEffect::createBlur(float radius) {
    std::vector<ShaderParam> params = {
        {"radius", radius, 0.0f, 20.0f, "Blur radius in pixels"}
    };

    auto effect = std::make_unique<ShaderEffect>(
        "Blur",
        ShaderEffectPresets::createBlurConfig(),
        params
    );
    effect->setParam("radius", radius);
    return effect;
}

std::unique_ptr<ShaderEffect> ShaderEffect::createSharpen(float amount) {
    std::vector<ShaderParam> params = {
        {"amount", amount, 0.0f, 5.0f, "Sharpen amount"}
    };

    auto effect = std::make_unique<ShaderEffect>(
        "Sharpen",
        ShaderEffectPresets::createSharpenConfig(),
        params
    );
    effect->setParam("amount", amount);
    return effect;
}

std::unique_ptr<ShaderEffect> ShaderEffect::createVignette() {
    std::vector<ShaderParam> params = {
        {"intensity", 0.5f, 0.0f, 1.0f, "Vignette intensity"},
        {"radius", 0.8f, 0.0f, 2.0f, "Vignette radius"}
    };

    return std::make_unique<ShaderEffect>(
        "Vignette",
        ShaderEffectPresets::createVignetteConfig(),
        params
    );
}

std::unique_ptr<ShaderEffect> ShaderEffect::createChromaticAberration() {
    std::vector<ShaderParam> params = {
        {"amount", 0.01f, 0.0f, 0.1f, "Chromatic aberration amount"}
    };

    return std::make_unique<ShaderEffect>(
        "ChromaticAberration",
        ShaderEffectPresets::createChromaticAberrationConfig(),
        params
    );
}

std::unique_ptr<ShaderEffect> ShaderEffect::createMouseSpotlight() {
    std::vector<ShaderParam> params = {
        {"iMouse", std::array<float, 4>{0.5f, 0.5f, 0.5f, 0.5f}, std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}, std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f}, "Mouse position (auto-filled)"},
        {"radius", 0.3f, 0.0f, 1.0f, "Spotlight radius"},
        {"intensity", 0.7f, 0.0f, 1.0f, "Darkness intensity"}
    };

    auto effect = std::make_unique<ShaderEffect>(
        "MouseSpotlight",
        ShaderEffectPresets::createMouseSpotlightConfig(),
        params
    );
    effect->enableInputBinding(true);  // Enable automatic input binding
    return effect;
}

std::unique_ptr<ShaderEffect> ShaderEffect::createPixelation() {
    std::vector<ShaderParam> params = {
        {"iTime", 0.0f, 0.0f, 1000.0f, "Time (auto-filled)"},
        {"pixelSize", 8.0f, 1.0f, 64.0f, "Pixel size"}
    };

    auto effect = std::make_unique<ShaderEffect>(
        "Pixelation",
        ShaderEffectPresets::createPixelationConfig(),
        params
    );
    effect->enableInputBinding(true);
    return effect;
}

// ============================================================================
// Shader Effect Presets
// ============================================================================

namespace ShaderEffectPresets {

ShaderConfig createColorAdjustConfig() {
    ShaderConfig config;
    config.name = "Color Adjust Effect";

    config.vertexShaderSource = R"(
        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @vertex
        fn vs(@location(0) pos : vec2f, @location(1) uv : vec2f) -> VertexOutput {
            var out : VertexOutput;
            out.pos = vec4f(pos, 0.0, 1.0);
            out.uv = uv;
            return out;
        }
    )";

    config.fragmentShaderSource = R"(
        @group(0) @binding(0) var mySampler : sampler;
        @group(0) @binding(1) var inputTexture : texture_2d<f32>;
        @group(0) @binding(2) var<uniform> params : vec4f; // brightness, contrast, hue, saturation

        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        fn rgb2hsv(rgb: vec3f) -> vec3f {
            let maxVal = max(max(rgb.r, rgb.g), rgb.b);
            let minVal = min(min(rgb.r, rgb.g), rgb.b);
            let delta = maxVal - minVal;
            var h = 0.0;
            var s = 0.0;
            let v = maxVal;
            if (delta > 0.00001) {
                s = delta / maxVal;
                if (rgb.r >= maxVal) {
                    h = (rgb.g - rgb.b) / delta;
                } else if (rgb.g >= maxVal) {
                    h = 2.0 + (rgb.b - rgb.r) / delta;
                } else {
                    h = 4.0 + (rgb.r - rgb.g) / delta;
                }
                h = h * 60.0;
                if (h < 0.0) { h = h + 360.0; }
            }
            return vec3f(h, s, v);
        }

        fn hsv2rgb(hsv: vec3f) -> vec3f {
            let h = hsv.x;
            let s = hsv.y;
            let v = hsv.z;
            if (s <= 0.0) { return vec3f(v, v, v); }
            var hh = h;
            if (hh >= 360.0) { hh = 0.0; }
            hh = hh / 60.0;
            let i = floor(hh);
            let ff = hh - i;
            let p = v * (1.0 - s);
            let q = v * (1.0 - (s * ff));
            let t = v * (1.0 - (s * (1.0 - ff)));
            if (i == 0.0) { return vec3f(v, t, p); }
            else if (i == 1.0) { return vec3f(q, v, p); }
            else if (i == 2.0) { return vec3f(p, v, t); }
            else if (i == 3.0) { return vec3f(p, q, v); }
            else if (i == 4.0) { return vec3f(t, p, v); }
            else { return vec3f(v, p, q); }
        }

        @fragment
        fn fs(input : VertexOutput) -> @location(0) vec4f {
            var color = textureSample(inputTexture, mySampler, input.uv);
            let brightness = params.x;
            let contrast = params.y;
            let hueShift = params.z;
            let saturation = params.w;

            var rgb = color.rgb + vec3f(brightness);
            rgb = (rgb - 0.5) * contrast + 0.5;
            var hsv = rgb2hsv(rgb);
            hsv.y = hsv.y * saturation;
            hsv.x = hsv.x + hueShift;
            if (hsv.x < 0.0) { hsv.x = hsv.x + 360.0; }
            else if (hsv.x >= 360.0) { hsv.x = hsv.x - 360.0; }
            rgb = hsv2rgb(hsv);
            return vec4f(clamp(rgb, vec3f(0.0), vec3f(1.0)), color.a);
        }
    )";

    config.bindings = {
        {0, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Sampler},
        {1, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Texture, wgpu::SamplerBindingType::Filtering, wgpu::TextureSampleType::Float},
        {2, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Buffer, wgpu::SamplerBindingType::Filtering, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e2D, wgpu::BufferBindingType::Uniform, false, 16}
    };

    config.vertexAttributes = {
        {wgpu::VertexFormat::Float32x2, 0, 0},
        {wgpu::VertexFormat::Float32x2, sizeof(float) * 2, 1}
    };
    config.vertexStride = sizeof(float) * 4;

    return config;
}

ShaderConfig createBlurConfig() {
    ShaderConfig config;
    config.name = "Gaussian Blur Effect";

    config.vertexShaderSource = R"(
        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @vertex
        fn vs(@location(0) pos : vec2f, @location(1) uv : vec2f) -> VertexOutput {
            var out : VertexOutput;
            out.pos = vec4f(pos, 0.0, 1.0);
            out.uv = uv;
            return out;
        }
    )";

    config.fragmentShaderSource = R"(
        @group(0) @binding(0) var mySampler : sampler;
        @group(0) @binding(1) var inputTexture : texture_2d<f32>;
        @group(0) @binding(2) var<uniform> params : vec4f; // radius, unused, unused, unused

        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @fragment
        fn fs(input : VertexOutput) -> @location(0) vec4f {
            let radius = params.x;
            let texSize = vec2f(textureDimensions(inputTexture));
            let texelSize = 1.0 / texSize;

            var result = vec4f(0.0);
            var totalWeight = 0.0;

            // Simple box blur
            let iRadius = i32(radius);
            for (var x = -iRadius; x <= iRadius; x++) {
                for (var y = -iRadius; y <= iRadius; y++) {
                    let offset = vec2f(f32(x), f32(y)) * texelSize;
                    let sample = textureSample(inputTexture, mySampler, input.uv + offset);
                    result += sample;
                    totalWeight += 1.0;
                }
            }

            return result / totalWeight;
        }
    )";

    config.bindings = {
        {0, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Sampler},
        {1, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Texture, wgpu::SamplerBindingType::Filtering, wgpu::TextureSampleType::Float},
        {2, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Buffer, wgpu::SamplerBindingType::Filtering, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e2D, wgpu::BufferBindingType::Uniform, false, 16}
    };

    config.vertexAttributes = {
        {wgpu::VertexFormat::Float32x2, 0, 0},
        {wgpu::VertexFormat::Float32x2, sizeof(float) * 2, 1}
    };
    config.vertexStride = sizeof(float) * 4;

    return config;
}

ShaderConfig createSharpenConfig() {
    ShaderConfig config;
    config.name = "Sharpen Effect";

    config.vertexShaderSource = R"(
        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @vertex
        fn vs(@location(0) pos : vec2f, @location(1) uv : vec2f) -> VertexOutput {
            var out : VertexOutput;
            out.pos = vec4f(pos, 0.0, 1.0);
            out.uv = uv;
            return out;
        }
    )";

    config.fragmentShaderSource = R"(
        @group(0) @binding(0) var mySampler : sampler;
        @group(0) @binding(1) var inputTexture : texture_2d<f32>;
        @group(0) @binding(2) var<uniform> params : vec4f; // amount, unused, unused, unused

        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @fragment
        fn fs(input : VertexOutput) -> @location(0) vec4f {
            let amount = params.x;
            let texSize = vec2f(textureDimensions(inputTexture));
            let texelSize = 1.0 / texSize;

            let center = textureSample(inputTexture, mySampler, input.uv);
            let top = textureSample(inputTexture, mySampler, input.uv + vec2f(0.0, -texelSize.y));
            let bottom = textureSample(inputTexture, mySampler, input.uv + vec2f(0.0, texelSize.y));
            let left = textureSample(inputTexture, mySampler, input.uv + vec2f(-texelSize.x, 0.0));
            let right = textureSample(inputTexture, mySampler, input.uv + vec2f(texelSize.x, 0.0));

            let edge = (top + bottom + left + right) * 0.25;
            let sharpened = center + (center - edge) * amount;

            return vec4f(clamp(sharpened.rgb, vec3f(0.0), vec3f(1.0)), center.a);
        }
    )";

    config.bindings = {
        {0, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Sampler},
        {1, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Texture, wgpu::SamplerBindingType::Filtering, wgpu::TextureSampleType::Float},
        {2, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Buffer, wgpu::SamplerBindingType::Filtering, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e2D, wgpu::BufferBindingType::Uniform, false, 16}
    };

    config.vertexAttributes = {
        {wgpu::VertexFormat::Float32x2, 0, 0},
        {wgpu::VertexFormat::Float32x2, sizeof(float) * 2, 1}
    };
    config.vertexStride = sizeof(float) * 4;

    return config;
}

ShaderConfig createVignetteConfig() {
    ShaderConfig config;
    config.name = "Vignette Effect";

    config.vertexShaderSource = R"(
        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @vertex
        fn vs(@location(0) pos : vec2f, @location(1) uv : vec2f) -> VertexOutput {
            var out : VertexOutput;
            out.pos = vec4f(pos, 0.0, 1.0);
            out.uv = uv;
            return out;
        }
    )";

    config.fragmentShaderSource = R"(
        @group(0) @binding(0) var mySampler : sampler;
        @group(0) @binding(1) var inputTexture : texture_2d<f32>;
        @group(0) @binding(2) var<uniform> params : vec4f; // intensity, radius, unused, unused

        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @fragment
        fn fs(input : VertexOutput) -> @location(0) vec4f {
            let color = textureSample(inputTexture, mySampler, input.uv);
            let intensity = params.x;
            let radius = params.y;

            let center = vec2f(0.5, 0.5);
            let dist = distance(input.uv, center);
            let vignette = smoothstep(radius, radius - 0.3, dist);
            let factor = mix(1.0 - intensity, 1.0, vignette);

            return vec4f(color.rgb * factor, color.a);
        }
    )";

    config.bindings = {
        {0, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Sampler},
        {1, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Texture, wgpu::SamplerBindingType::Filtering, wgpu::TextureSampleType::Float},
        {2, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Buffer, wgpu::SamplerBindingType::Filtering, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e2D, wgpu::BufferBindingType::Uniform, false, 16}
    };

    config.vertexAttributes = {
        {wgpu::VertexFormat::Float32x2, 0, 0},
        {wgpu::VertexFormat::Float32x2, sizeof(float) * 2, 1}
    };
    config.vertexStride = sizeof(float) * 4;

    return config;
}

ShaderConfig createChromaticAberrationConfig() {
    ShaderConfig config;
    config.name = "Chromatic Aberration Effect";

    config.vertexShaderSource = R"(
        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @vertex
        fn vs(@location(0) pos : vec2f, @location(1) uv : vec2f) -> VertexOutput {
            var out : VertexOutput;
            out.pos = vec4f(pos, 0.0, 1.0);
            out.uv = uv;
            return out;
        }
    )";

    config.fragmentShaderSource = R"(
        @group(0) @binding(0) var mySampler : sampler;
        @group(0) @binding(1) var inputTexture : texture_2d<f32>;
        @group(0) @binding(2) var<uniform> params : vec4f; // amount, unused, unused, unused

        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @fragment
        fn fs(input : VertexOutput) -> @location(0) vec4f {
            let amount = params.x;
            let center = vec2f(0.5, 0.5);
            let offset = (input.uv - center) * amount;

            let r = textureSample(inputTexture, mySampler, input.uv + offset).r;
            let g = textureSample(inputTexture, mySampler, input.uv).g;
            let b = textureSample(inputTexture, mySampler, input.uv - offset).b;
            let a = textureSample(inputTexture, mySampler, input.uv).a;

            return vec4f(r, g, b, a);
        }
    )";

    config.bindings = {
        {0, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Sampler},
        {1, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Texture, wgpu::SamplerBindingType::Filtering, wgpu::TextureSampleType::Float},
        {2, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Buffer, wgpu::SamplerBindingType::Filtering, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e2D, wgpu::BufferBindingType::Uniform, false, 16}
    };

    config.vertexAttributes = {
        {wgpu::VertexFormat::Float32x2, 0, 0},
        {wgpu::VertexFormat::Float32x2, sizeof(float) * 2, 1}
    };
    config.vertexStride = sizeof(float) * 4;

    return config;
}

ShaderConfig createMouseSpotlightConfig() {
    ShaderConfig config;
    config.name = "Mouse Spotlight Effect";

    config.vertexShaderSource = R"(
        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @vertex
        fn vs(@location(0) pos : vec2f, @location(1) uv : vec2f) -> VertexOutput {
            var out : VertexOutput;
            out.pos = vec4f(pos, 0.0, 1.0);
            out.uv = uv;
            return out;
        }
    )";

    config.fragmentShaderSource = R"(
        @group(0) @binding(0) var mySampler : sampler;
        @group(0) @binding(1) var inputTexture : texture_2d<f32>;
        @group(0) @binding(2) var<uniform> params : vec4f; // iMouse.xy (first 2), radius, intensity

        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @fragment
        fn fs(input : VertexOutput) -> @location(0) vec4f {
            let color = textureSample(inputTexture, mySampler, input.uv);
            let mousePos = vec2f(params.x, params.y);
            let radius = params.z;
            let intensity = params.w;

            let dist = distance(input.uv, mousePos);
            let spotlight = smoothstep(radius, radius * 0.5, dist);
            let darken = mix(intensity, 1.0, spotlight);

            return vec4f(color.rgb * darken, color.a);
        }
    )";

    config.bindings = {
        {0, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Sampler},
        {1, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Texture,
         wgpu::SamplerBindingType::Filtering, wgpu::TextureSampleType::Float},
        {2, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Buffer,
         wgpu::SamplerBindingType::Filtering, wgpu::TextureSampleType::Float,
         wgpu::TextureViewDimension::e2D, wgpu::BufferBindingType::Uniform, false, 16}
    };

    config.vertexAttributes = {
        {wgpu::VertexFormat::Float32x2, 0, 0},
        {wgpu::VertexFormat::Float32x2, sizeof(float) * 2, 1}
    };
    config.vertexStride = sizeof(float) * 4;

    return config;
}

ShaderConfig createPixelationConfig() {
    ShaderConfig config;
    config.name = "Pixelation Effect";

    config.vertexShaderSource = R"(
        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @vertex
        fn vs(@location(0) pos : vec2f, @location(1) uv : vec2f) -> VertexOutput {
            var out : VertexOutput;
            out.pos = vec4f(pos, 0.0, 1.0);
            out.uv = uv;
            return out;
        }
    )";

    config.fragmentShaderSource = R"(
        @group(0) @binding(0) var mySampler : sampler;
        @group(0) @binding(1) var inputTexture : texture_2d<f32>;
        @group(0) @binding(2) var<uniform> params : vec4f; // iTime, pixelSize, unused, unused

        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @fragment
        fn fs(input : VertexOutput) -> @location(0) vec4f {
            let time = params.x;
            let pixelSize = params.y;

            // Animate pixel size with time
            let animatedSize = pixelSize * (1.0 + 0.3 * sin(time * 0.5));

            let texSize = vec2f(textureDimensions(inputTexture));
            let pixelated = floor(input.uv * texSize / animatedSize) * animatedSize / texSize;

            return textureSample(inputTexture, mySampler, pixelated);
        }
    )";

    config.bindings = {
        {0, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Sampler},
        {1, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Texture,
         wgpu::SamplerBindingType::Filtering, wgpu::TextureSampleType::Float},
        {2, wgpu::ShaderStage::Fragment, ShaderBindingDesc::Type::Buffer,
         wgpu::SamplerBindingType::Filtering, wgpu::TextureSampleType::Float,
         wgpu::TextureViewDimension::e2D, wgpu::BufferBindingType::Uniform, false, 16}
    };

    config.vertexAttributes = {
        {wgpu::VertexFormat::Float32x2, 0, 0},
        {wgpu::VertexFormat::Float32x2, sizeof(float) * 2, 1}
    };
    config.vertexStride = sizeof(float) * 4;

    return config;
}

} // namespace ShaderEffectPresets
