#include "ShaderConfig.h"

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
