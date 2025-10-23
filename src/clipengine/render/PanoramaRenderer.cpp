#include "PanoramaRenderer.h"
#include "ShaderConfig.h"
#include <iostream>

static ShaderConfig createPanoramaShader() {
    ShaderConfig cfg;
    cfg.name = "Panorama Planar Shader";

    cfg.vertexShaderSource = R"(
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

    // Fragment shader: map planar UV -> spherical sample from equirectangular texture
    cfg.fragmentShaderSource = R"(
        @group(0) @binding(0) var mySampler : sampler;
        @group(0) @binding(1) var myTexture : texture_2d<f32>;
    @group(0) @binding(2) var<uniform> u : vec4f; // yaw, pitch, zoom, aspect

        struct VertexOutput {
            @builtin(position) pos : vec4f,
            @location(0) uv : vec2f
        };

        fn toSpherical(uv: vec2f, yaw: f32, pitch: f32, zoom: f32, aspect: f32) -> vec2f {
            // convert to centered NDC-like coordinates
            var x = (uv.x - 0.5) * aspect;
            var y = (uv.y - 0.5);
            // apply zoom (field of view)
            x = x / zoom;
            y = y / zoom;

            // create camera ray in camera space (z = -1 forward)
            var dir = vec3f(x, y, -1.0);
            dir = normalize(dir);

            // rotate by pitch (x axis) and yaw (y axis)
            // yaw rotation around world up (y)
            let cy = cos(yaw);
            let sy = sin(yaw);
            let cx = cos(pitch);
            let sx = sin(pitch);

            var rx = vec3f(dir.x, dir.y * cx - dir.z * sx, dir.y * sx + dir.z * cx);
            var r = vec3f(rx.x * cy + rx.z * sy, rx.y, -rx.x * sy + rx.z * cy);

            // spherical coords
            let lon = atan2(r.x, -r.z);
            let lat = asin(clamp(r.y, -1.0, 1.0));

            var uout = lon / (2.0 * 3.14159265) + 0.5;
            var vout = 0.5 - lat / 3.14159265;
            return vec2f(fract(uout), clamp(vout, 0.0, 1.0));
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

    // bindings: sampler + texture + uniform buffer
    cfg.bindings = {
        ShaderBindingDesc{.binding = 0, .visibility = wgpu::ShaderStage::Fragment, .type = ShaderBindingDesc::Type::Sampler},
        ShaderBindingDesc{.binding = 1, .visibility = wgpu::ShaderStage::Fragment, .type = ShaderBindingDesc::Type::Texture, .textureSampleType = wgpu::TextureSampleType::Float, .textureViewDimension = wgpu::TextureViewDimension::e2D},
        ShaderBindingDesc{.binding = 2, .visibility = wgpu::ShaderStage::Fragment, .type = ShaderBindingDesc::Type::Buffer, .bufferType = wgpu::BufferBindingType::Uniform, .hasDynamicOffset = false, .minBindingSize = 16}
    };

    cfg.vertexAttributes = {
        ShaderConfig::VertexAttribute{.format = wgpu::VertexFormat::Float32x2, .offset = 0, .shaderLocation = 0},
        ShaderConfig::VertexAttribute{.format = wgpu::VertexFormat::Float32x2, .offset = sizeof(float)*2, .shaderLocation = 1}
    };
    cfg.vertexStride = sizeof(float) * 4;

    return cfg;
}

static ShaderConfig createPanoramaNV12Shader() {
    ShaderConfig cfg;
    cfg.name = "Panorama NV12 Shader";

    cfg.vertexShaderSource = R"(
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

    // Fragment shader: sample NV12 (Y + UV) and map to spherical UV
    cfg.fragmentShaderSource = R"(
        @group(0) @binding(0) var mySampler : sampler;
        @group(0) @binding(1) var yTex : texture_2d<f32>;
        @group(0) @binding(2) var uvTex : texture_2d<f32>;
    @group(0) @binding(3) var<uniform> u : vec4f; // yaw, pitch, zoom, aspect

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
            return vec2f(fract(uout), clamp(vout, 0.0, 1.0));
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

    cfg.bindings = {
        ShaderBindingDesc{.binding = 0, .visibility = wgpu::ShaderStage::Fragment, .type = ShaderBindingDesc::Type::Sampler},
        ShaderBindingDesc{.binding = 1, .visibility = wgpu::ShaderStage::Fragment, .type = ShaderBindingDesc::Type::Texture, .textureSampleType = wgpu::TextureSampleType::Float, .textureViewDimension = wgpu::TextureViewDimension::e2D},
        ShaderBindingDesc{.binding = 2, .visibility = wgpu::ShaderStage::Fragment, .type = ShaderBindingDesc::Type::Texture, .textureSampleType = wgpu::TextureSampleType::Float, .textureViewDimension = wgpu::TextureViewDimension::e2D},
        ShaderBindingDesc{.binding = 3, .visibility = wgpu::ShaderStage::Fragment, .type = ShaderBindingDesc::Type::Buffer, .bufferType = wgpu::BufferBindingType::Uniform, .hasDynamicOffset = false, .minBindingSize = 16}
    };

    cfg.vertexAttributes = {
        ShaderConfig::VertexAttribute{.format = wgpu::VertexFormat::Float32x2, .offset = 0, .shaderLocation = 0},
        ShaderConfig::VertexAttribute{.format = wgpu::VertexFormat::Float32x2, .offset = sizeof(float)*2, .shaderLocation = 1}
    };
    cfg.vertexStride = sizeof(float) * 4;

    return cfg;
}

PanoramaRenderer::PanoramaRenderer()
    : TextureRenderer(createPanoramaShader()) {
}

PanoramaRenderer::~PanoramaRenderer() = default;

bool PanoramaRenderer::initialize(wgpu::Device device, wgpu::TextureFormat format) {
    bool ok = TextureRenderer::initialize(device, format);
    // ensure uniform buffer exists
    updateUniformsIfNeeded();
    return ok;
}

void PanoramaRenderer::updateUniformsIfNeeded() {
    if (!uniformsDirty_) return;
    updateUniformData(&uniforms_, sizeof(uniforms_));
    uniformsDirty_ = false;
}

void PanoramaRenderer::setRotation(float yawRadians, float pitchRadians) {
    uniforms_.yaw = yawRadians;
    uniforms_.pitch = pitchRadians;
    uniformsDirty_ = true;
}

void PanoramaRenderer::setZoom(float zoom) {
    uniforms_.zoom = zoom;
    uniformsDirty_ = true;
}

void PanoramaRenderer::update(float deltaTime) {
    updateUniformsIfNeeded();
}

void PanoramaRenderer::render(wgpu::RenderPassEncoder& pass) {
    // ensure uniforms are uploaded before render
    updateUniformsIfNeeded();
    TextureRenderer::render(pass);
}

void PanoramaRenderer::applyTextureViews(const std::vector<wgpu::TextureView>& views) {
    // Determine shader based on number of views: NV12 -> 2 views (Y, UV); RGBA -> 1 view
    if (views.size() == 2) {
        shaderConfig_ = createPanoramaNV12Shader();
    } else {
        shaderConfig_ = createPanoramaShader();
    }

    // If device is available, recreate shader modules and pipeline
    if (device_) {
        initializeShader();
        initializePipeline();
    }

    // Update textures via base class
    updateTextures(views);

    // Ensure uniforms are uploaded
    updateUniformsIfNeeded();
}
