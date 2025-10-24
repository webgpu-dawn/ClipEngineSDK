#include "ShaderConfig.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

// ============================================================================
// ShaderConfig File Loading
// ============================================================================

std::string ShaderConfig::loadShaderFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filepath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// NOTE: ShaderFactory and ShaderBindingBuilder have been moved to
// shader/ShaderLibrary.cpp for better organization.

// ============================================================================
// ShaderPresets - Legacy Inline Shaders (for compatibility)
// ============================================================================

ShaderConfig ShaderPresets::createNV12VideoShader() {
    ShaderConfig config;
    config.name = "NV12 Video Shader";

    // Vertex shader (共通)
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

    // Fragment shader (NV12 specific)
    config.fragmentShaderSource = R"(
        @group(0) @binding(0) var mySampler : sampler;
        @group(0) @binding(1) var yTex : texture_2d<f32>;
        @group(0) @binding(2) var uvTex : texture_2d<f32>;

        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @fragment
        fn fs(input : VertexOutput) -> @location(0) vec4f {
            let y = textureSample(yTex, mySampler, input.uv).r;
            let uv = textureSample(uvTex, mySampler, input.uv).rg;

            let u = uv.r - 0.5;
            let v = uv.g - 0.5;

            var rgb : vec3f;
            rgb.r = y + 1.5748 * v;
            rgb.g = y - 0.1873 * u - 0.4681 * v;
            rgb.b = y + 1.8556 * u;

            return vec4f(clamp(rgb, vec3f(0.0), vec3f(1.0)), 1.0);
        }
    )";

    // Bindings: sampler + 2 textures (Y and UV)
    config.bindings = {
        ShaderBindingDesc{
            .binding = 0,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Sampler,
            .samplerType = wgpu::SamplerBindingType::Filtering
        },
        ShaderBindingDesc{
            .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Texture,
            .textureSampleType = wgpu::TextureSampleType::Float,
            .textureViewDimension = wgpu::TextureViewDimension::e2D
        },
        ShaderBindingDesc{
            .binding = 2,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Texture,
            .textureSampleType = wgpu::TextureSampleType::Float,
            .textureViewDimension = wgpu::TextureViewDimension::e2D
        }
    };

    // Vertex attributes: position + uv
    config.vertexAttributes = {
        ShaderConfig::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x2,
            .offset = 0,
            .shaderLocation = 0
        },
        ShaderConfig::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x2,
            .offset = sizeof(float) * 2,
            .shaderLocation = 1
        }
    };
    config.vertexStride = sizeof(float) * 4;

    return config;
}

ShaderConfig ShaderPresets::createRGBATextureShader() {
    ShaderConfig config;
    config.name = "RGBA Texture Shader";

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
        @group(0) @binding(1) var myTexture : texture_2d<f32>;

        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @fragment
        fn fs(input : VertexOutput) -> @location(0) vec4f {
            return textureSample(myTexture, mySampler, input.uv);
        }
    )";

    // Bindings: sampler + 1 texture
    config.bindings = {
        ShaderBindingDesc{
            .binding = 0,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Sampler,
            .samplerType = wgpu::SamplerBindingType::Filtering
        },
        ShaderBindingDesc{
            .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Texture,
            .textureSampleType = wgpu::TextureSampleType::Float,
            .textureViewDimension = wgpu::TextureViewDimension::e2D
        }
    };

    // Vertex attributes
    config.vertexAttributes = {
        ShaderConfig::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x2,
            .offset = 0,
            .shaderLocation = 0
        },
        ShaderConfig::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x2,
            .offset = sizeof(float) * 2,
            .shaderLocation = 1
        }
    };
    config.vertexStride = sizeof(float) * 4;

    return config;
}

ShaderConfig ShaderPresets::createI420VideoShader() {
    ShaderConfig config;
    config.name = "I420 Video Shader";

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
        @group(0) @binding(1) var yTex : texture_2d<f32>;
        @group(0) @binding(2) var uTex : texture_2d<f32>;
        @group(0) @binding(3) var vTex : texture_2d<f32>;

        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @fragment
        fn fs(input : VertexOutput) -> @location(0) vec4f {
            let y = textureSample(yTex, mySampler, input.uv).r;
            let u = textureSample(uTex, mySampler, input.uv).r - 0.5;
            let v = textureSample(vTex, mySampler, input.uv).r - 0.5;

            var rgb : vec3f;
            rgb.r = y + 1.5748 * v;
            rgb.g = y - 0.1873 * u - 0.4681 * v;
            rgb.b = y + 1.8556 * u;

            return vec4f(clamp(rgb, vec3f(0.0), vec3f(1.0)), 1.0);
        }
    )";

    // Bindings: sampler + 3 textures (Y, U, V)
    config.bindings = {
        ShaderBindingDesc{
            .binding = 0,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Sampler,
            .samplerType = wgpu::SamplerBindingType::Filtering
        },
        ShaderBindingDesc{
            .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Texture,
            .textureSampleType = wgpu::TextureSampleType::Float,
            .textureViewDimension = wgpu::TextureViewDimension::e2D
        },
        ShaderBindingDesc{
            .binding = 2,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Texture,
            .textureSampleType = wgpu::TextureSampleType::Float,
            .textureViewDimension = wgpu::TextureViewDimension::e2D
        },
        ShaderBindingDesc{
            .binding = 3,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Texture,
            .textureSampleType = wgpu::TextureSampleType::Float,
            .textureViewDimension = wgpu::TextureViewDimension::e2D
        }
    };

    // Vertex attributes
    config.vertexAttributes = {
        ShaderConfig::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x2,
            .offset = 0,
            .shaderLocation = 0
        },
        ShaderConfig::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x2,
            .offset = sizeof(float) * 2,
            .shaderLocation = 1
        }
    };
    config.vertexStride = sizeof(float) * 4;

    return config;
}

ShaderConfig ShaderPresets::createColorShader() {
    ShaderConfig config;
    config.name = "Simple Color Shader";

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
        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        @fragment
        fn fs(input : VertexOutput) -> @location(0) vec4f {
            return vec4f(input.uv, 0.5, 1.0);
        }
    )";

    // No bindings needed for simple color shader
    config.bindings = {};

    // Vertex attributes
    config.vertexAttributes = {
        ShaderConfig::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x2,
            .offset = 0,
            .shaderLocation = 0
        },
        ShaderConfig::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x2,
            .offset = sizeof(float) * 2,
            .shaderLocation = 1
        }
    };
    config.vertexStride = sizeof(float) * 4;

    return config;
}

ShaderConfig ShaderPresets::createPanoramaRGBAShader() {
    // NOTE: This function is deprecated. Use clipengine::ShaderLibrary::create() instead.
    // For compatibility, we inline the panorama RGBA shader here.
    ShaderConfig config;
    config.name = "Panorama RGBA Shader (Legacy)";

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
        @group(0) @binding(1) var myTexture : texture_2d<f32>;
        @group(0) @binding(2) var<uniform> u : vec4f;
        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };
        fn toSpherical(uv: vec2f, yaw: f32, pitch: f32, zoom: f32, aspect: f32) -> vec2f {
            var x = (uv.x - 0.5) * aspect;
            var y = (uv.y - 0.5);
            x = x / zoom;
            y = y / zoom;
            var dir = vec3f(x, y, -1.0);
            dir = normalize(dir);
            let cy = cos(yaw);
            let sy = sin(yaw);
            let cx = cos(pitch);
            let sx = sin(pitch);
            var rx = vec3f(dir.x, dir.y * cx - dir.z * sx, dir.y * sx + dir.z * cx);
            var r = vec3f(rx.x * cy + rx.z * sy, rx.y, -rx.x * sy + rx.z * cy);
            let lon = atan2(r.x, -r.z);
            let lat = asin(clamp(r.y, -1.0, 1.0));
            var uout = lon / (2.0 * 3.14159265) + 0.5;
            var vout = 0.5 - lat / 3.14159265;
            return vec2f(fract(uout), clamp(1.0 - vout, 0.0, 1.0));
        }
        @fragment
        fn fs(input : VertexOutput) -> @location(0) vec4f {
            let yaw = u.x;
            let pitch = u.y;
            let zoom = max(u.z, 0.01);
            let aspect = max(u.w, 1.0);
            let sphUV = toSpherical(input.uv, yaw, pitch, zoom, aspect);
            return textureSample(myTexture, mySampler, sphUV);
        }
    )";

    config.bindings = {
        ShaderBindingDesc{.binding = 0, .visibility = wgpu::ShaderStage::Fragment, .type = ShaderBindingDesc::Type::Sampler},
        ShaderBindingDesc{.binding = 1, .visibility = wgpu::ShaderStage::Fragment, .type = ShaderBindingDesc::Type::Texture},
        ShaderBindingDesc{.binding = 2, .visibility = wgpu::ShaderStage::Fragment, .type = ShaderBindingDesc::Type::Buffer, .bufferType = wgpu::BufferBindingType::Uniform, .minBindingSize = 16}
    };

    config.vertexAttributes = {
        ShaderConfig::VertexAttribute{.format = wgpu::VertexFormat::Float32x2, .offset = 0, .shaderLocation = 0},
        ShaderConfig::VertexAttribute{.format = wgpu::VertexFormat::Float32x2, .offset = sizeof(float)*2, .shaderLocation = 1}
    };
    config.vertexStride = sizeof(float) * 4;

    return config;
}

ShaderConfig ShaderPresets::createPanoramaNV12Shader() {
    // NOTE: This function is deprecated. Use clipengine::ShaderLibrary::create() instead.
    ShaderConfig config;
    config.name = "Panorama NV12 Shader (Legacy)";

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
        @group(0) @binding(1) var yTex : texture_2d<f32>;
        @group(0) @binding(2) var uvTex : texture_2d<f32>;
        @group(0) @binding(3) var<uniform> u : vec4f;
        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };
        fn toSpherical(uv: vec2f, yaw: f32, pitch: f32, zoom: f32, aspect: f32) -> vec2f {
            var x = (uv.x - 0.5) * aspect;
            var y = (uv.y - 0.5);
            x = x / zoom;
            y = y / zoom;
            var dir = vec3f(x, y, -1.0);
            dir = normalize(dir);
            let cy = cos(yaw);
            let sy = sin(yaw);
            let cx = cos(pitch);
            let sx = sin(pitch);
            var rx = vec3f(dir.x, dir.y * cx - dir.z * sx, dir.y * sx + dir.z * cx);
            var r = vec3f(rx.x * cy + rx.z * sy, rx.y, -rx.x * sy + rx.z * cy);
            let lon = atan2(r.x, -r.z);
            let lat = asin(clamp(r.y, -1.0, 1.0));
            var uout = lon / (2.0 * 3.14159265) + 0.5;
            var vout = 0.5 - lat / 3.14159265;
            return vec2f(fract(uout), clamp(1.0 - vout, 0.0, 1.0));
        }
        @fragment
        fn fs(input : VertexOutput) -> @location(0) vec4f {
            let yaw = u.x;
            let pitch = u.y;
            let zoom = max(u.z, 0.01);
            let aspect = max(u.w, 1.0);
            let sphUV = toSpherical(input.uv, yaw, pitch, zoom, aspect);
            let y = textureSample(yTex, mySampler, sphUV).r;
            let uv = textureSample(uvTex, mySampler, sphUV).rg;
            let uval = uv.r - 0.5;
            let vval = uv.g - 0.5;
            var rgb : vec3f;
            rgb.r = y + 1.5748 * vval;
            rgb.g = y - 0.1873 * uval - 0.4681 * vval;
            rgb.b = y + 1.8556 * uval;
            return vec4f(clamp(rgb, vec3f(0.0), vec3f(1.0)), 1.0);
        }
    )";

    config.bindings = {
        ShaderBindingDesc{.binding = 0, .visibility = wgpu::ShaderStage::Fragment, .type = ShaderBindingDesc::Type::Sampler},
        ShaderBindingDesc{.binding = 1, .visibility = wgpu::ShaderStage::Fragment, .type = ShaderBindingDesc::Type::Texture},
        ShaderBindingDesc{.binding = 2, .visibility = wgpu::ShaderStage::Fragment, .type = ShaderBindingDesc::Type::Texture},
        ShaderBindingDesc{.binding = 3, .visibility = wgpu::ShaderStage::Fragment, .type = ShaderBindingDesc::Type::Buffer, .bufferType = wgpu::BufferBindingType::Uniform, .minBindingSize = 16}
    };

    config.vertexAttributes = {
        ShaderConfig::VertexAttribute{.format = wgpu::VertexFormat::Float32x2, .offset = 0, .shaderLocation = 0},
        ShaderConfig::VertexAttribute{.format = wgpu::VertexFormat::Float32x2, .offset = sizeof(float)*2, .shaderLocation = 1}
    };
    config.vertexStride = sizeof(float) * 4;

    return config;
}
