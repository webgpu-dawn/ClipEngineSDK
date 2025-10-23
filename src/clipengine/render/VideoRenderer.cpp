#include "VideoRenderer.h"

#if _WIN32
#include <dawn/native/D3D11Backend.h>
#include <dawn/native/D3D12Backend.h>
#elif __APPLE__
#endif
#include <iostream>

VideoRenderer::VideoRenderer()
    : TextureRenderer(ShaderPresets::createNV12VideoShader())
    , videoFormat_(VideoFormat::NV12) {
}

VideoRenderer::VideoRenderer(const ShaderConfig& config)
    : TextureRenderer(config) {
}

VideoRenderer::VideoRenderer(VideoFormat format)
    : TextureRenderer(ShaderPresets::createNV12VideoShader())  // Temporary, will be updated
    , videoFormat_(format) {
    createShaderForFormat();
}

VideoRenderer::~VideoRenderer() = default;

void VideoRenderer::createShaderForFormat() {
    switch (videoFormat_) {
        case VideoFormat::NV12:
            shaderConfig_ = ShaderPresets::createNV12VideoShader();
            break;
        case VideoFormat::I420:
            shaderConfig_ = ShaderPresets::createI420VideoShader();
            break;
        case VideoFormat::RGBA:
            shaderConfig_ = ShaderPresets::createRGBATextureShader();
            break;
    }
}

void VideoRenderer::setVideoFormat(VideoFormat format) {
    if (videoFormat_ == format) return;

    videoFormat_ = format;
    createShaderForFormat();

    // Reinitialize pipeline with new shader
    if (device_) {
        initializeShader();
        initializePipeline();
    }
}

namespace {
    struct SharedTextureData {
        ComPtr<ID3D11Texture2D> texture;
        HANDLE handle = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    bool CreateD3D11SharedTexture(ComPtr<ID3D11Device>& device, const D3D11_TEXTURE2D_DESC& srcDesc, SharedTextureData& outData) {
        D3D11_TEXTURE2D_DESC desc = {
            .Width       = srcDesc.Width,
            .Height      = srcDesc.Height,
            .MipLevels   = 1,
            .ArraySize   = 1,
            .Format      = (srcDesc.Format == DXGI_FORMAT_NV12 || srcDesc.Format == 103) ? DXGI_FORMAT_NV12 : srcDesc.Format,
            .SampleDesc  = {
                .Count   = 1,
                .Quality = 0
            },
            .Usage       = D3D11_USAGE_DEFAULT,
            .BindFlags   = 0,
            .CPUAccessFlags = 0,
            .MiscFlags   = D3D11_RESOURCE_MISC_SHARED_NTHANDLE
        };

        HRESULT hr = device->CreateTexture2D(&desc, nullptr, outData.texture.GetAddressOf());
        if(FAILED(hr)) {
            desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
            hr = device->CreateTexture2D(&desc, nullptr, outData.texture.GetAddressOf());
            if(FAILED(hr)) return false;
        }

        D3D11_TEXTURE2D_DESC createdDesc;
        outData.texture->GetDesc(&createdDesc);
        bool isNTHandle = (createdDesc.MiscFlags & D3D11_RESOURCE_MISC_SHARED_NTHANDLE) != 0;

        if(isNTHandle) {
            ComPtr<IDXGIResource1> dxgiRes;
            if(FAILED(outData.texture.As(&dxgiRes))) return false;
            if(FAILED(dxgiRes->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr, &outData.handle))) return false;
        } else {
            ComPtr<IDXGIResource> dxgiRes;
            if(FAILED(outData.texture.As(&dxgiRes))) return false;
            if(FAILED(dxgiRes->GetSharedHandle(&outData.handle))) return false;
        }

        outData.width = srcDesc.Width;
        outData.height = srcDesc.Height;
        return true;
    }

    struct DawnTextureData {
        wgpu::Texture texture;
        wgpu::SharedTextureMemory sharedMemory;
    };

    DawnTextureData ImportToDawnTexture(wgpu::Device& device, HANDLE sharedHandle) {
        DawnTextureData result;

        wgpu::SharedTextureMemoryDXGISharedHandleDescriptor handleDesc = {};
        handleDesc.handle = sharedHandle;
        handleDesc.useKeyedMutex = false;

        wgpu::SharedTextureMemoryDescriptor stmDesc = {};
        stmDesc.nextInChain = &handleDesc;

        result.sharedMemory = device.ImportSharedTextureMemory(&stmDesc);
        if(!result.sharedMemory) return {};

        wgpu::SharedTextureMemoryProperties props = {};
        result.sharedMemory.GetProperties(&props);

        wgpu::TextureDescriptor texDesc = {
            .usage         = props.usage,
            .dimension     = wgpu::TextureDimension::e2D,
            .size          = props.size,
            .format        = props.format,
            .mipLevelCount = 1,
            .sampleCount   = 1
        };

        result.texture = result.sharedMemory.CreateTexture(&texDesc);

        wgpu::SharedTextureMemoryBeginAccessDescriptor beginDesc = {};
        beginDesc.initialized = true;
        result.sharedMemory.BeginAccess(result.texture, &beginDesc);

        return result;
    }
}

bool VideoRenderer::updateFrame(ID3D11Texture2D* texture, int arrayIndex) {
    if(!texture) return false;

    ComPtr<ID3D11Device> d3d11Device;
    texture->GetDevice(d3d11Device.GetAddressOf());
    ComPtr<ID3D11DeviceContext> ctx;
    d3d11Device->GetImmediateContext(ctx.GetAddressOf());

    D3D11_TEXTURE2D_DESC srcDesc = {};
    texture->GetDesc(&srcDesc);

    static SharedTextureData sharedData;

    if(!sharedData.texture || sharedData.width != srcDesc.Width || sharedData.height != srcDesc.Height) {
        if(sharedData.handle) CloseHandle(sharedData.handle);
        sharedData = {};
        if(!CreateD3D11SharedTexture(d3d11Device, srcDesc, sharedData)) return false;
    }

    ctx->CopySubresourceRegion(sharedData.texture.Get(), 0, 0, 0, 0, texture, arrayIndex, nullptr);
    ctx->Flush();

    static DawnTextureData dawnData;
    static uint32_t lastWidth = 0, lastHeight = 0;

    if(!dawnData.texture || lastWidth != srcDesc.Width || lastHeight != srcDesc.Height) {
        dawnData = ImportToDawnTexture(device_, sharedData.handle);
        if(!dawnData.texture) return false;
        lastWidth = srcDesc.Width;
        lastHeight = srcDesc.Height;
    }

    // Create texture views based on video format
    std::vector<wgpu::TextureView> views;

    switch (videoFormat_) {
        case VideoFormat::NV12: {
            wgpu::TextureViewDescriptor yViewDesc = {
                .format          = wgpu::TextureFormat::R8Unorm,
                .dimension       = wgpu::TextureViewDimension::e2D,
                .baseMipLevel    = 0,
                .mipLevelCount   = 1,
                .baseArrayLayer  = 0,
                .arrayLayerCount = 1,
                .aspect          = wgpu::TextureAspect::Plane0Only
            };
            yPlaneView_ = dawnData.texture.CreateView(&yViewDesc);

            wgpu::TextureViewDescriptor uvViewDesc = {
                .format          = wgpu::TextureFormat::RG8Unorm,
                .dimension       = wgpu::TextureViewDimension::e2D,
                .baseMipLevel    = 0,
                .mipLevelCount   = 1,
                .baseArrayLayer  = 0,
                .arrayLayerCount = 1,
                .aspect          = wgpu::TextureAspect::Plane1Only
            };
            uvPlaneView_ = dawnData.texture.CreateView(&uvViewDesc);

            views = {yPlaneView_, uvPlaneView_};
            break;
        }
        case VideoFormat::I420: {
            // TODO: Implement I420 texture view creation
            // This would require 3 separate planes (Y, U, V)
            break;
        }
        case VideoFormat::RGBA: {
            wgpu::TextureViewDescriptor rgbaViewDesc = {
                .format          = wgpu::TextureFormat::RGBA8Unorm,
                .dimension       = wgpu::TextureViewDimension::e2D,
                .baseMipLevel    = 0,
                .mipLevelCount   = 1,
                .baseArrayLayer  = 0,
                .arrayLayerCount = 1
            };
            yPlaneView_ = dawnData.texture.CreateView(&rgbaViewDesc);
            views = {yPlaneView_};
            break;
        }
    }

    // Update textures using base class method
    updateTextures(views);

    return true;
}

// ============================================================================
// Panorama Mode Support
// ============================================================================

namespace {
    // Create panorama shader for single-plane texture (RGBA)
    ShaderConfig createPanoramaShaderRGBA() {
        ShaderConfig cfg;
        cfg.name = "Panorama RGBA Shader";

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

        cfg.fragmentShaderSource = R"(
            @group(0) @binding(0) var mySampler : sampler;
            @group(0) @binding(1) var myTexture : texture_2d<f32>;
            @group(0) @binding(2) var<uniform> u : vec4f; // yaw, pitch, zoom, aspect

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

    // Create panorama shader for NV12 format
    ShaderConfig createPanoramaShaderNV12() {
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
}

void VideoRenderer::createShaderForMode() {
    if (renderMode_ == RenderMode::Panorama) {
        // Panorama mode: use panorama shaders
        switch (videoFormat_) {
            case VideoFormat::NV12:
                shaderConfig_ = createPanoramaShaderNV12();
                break;
            case VideoFormat::RGBA:
                shaderConfig_ = createPanoramaShaderRGBA();
                break;
            case VideoFormat::I420:
                // TODO: Implement I420 panorama shader
                shaderConfig_ = createPanoramaShaderNV12();
                break;
        }
    } else {
        // Planar mode: use standard video shaders
        createShaderForFormat();
    }
}

void VideoRenderer::setRenderMode(RenderMode mode) {
    if (renderMode_ == mode) return;

    renderMode_ = mode;
    createShaderForMode();

    // Recreate pipeline with new shader
    if (device_) {
        initializeShader();
        initializePipeline();
    }

    panoramaUniformsDirty_ = true;
}

void VideoRenderer::setRotation(float yawRadians, float pitchRadians) {
    panoramaParams_.yaw = yawRadians;
    panoramaParams_.pitch = pitchRadians;
    panoramaUniformsDirty_ = true;
}

void VideoRenderer::setZoom(float zoom) {
    panoramaParams_.zoom = zoom;
    panoramaUniformsDirty_ = true;
}

void VideoRenderer::setAspect(float aspect) {
    panoramaParams_.aspect = aspect;
    panoramaUniformsDirty_ = true;
}

void VideoRenderer::updatePanoramaUniforms() {
    if (!panoramaUniformsDirty_) return;

    // Pack panorama parameters into uniform buffer
    float uniforms[4] = {
        panoramaParams_.yaw,
        panoramaParams_.pitch,
        panoramaParams_.zoom,
        panoramaParams_.aspect
    };

    updateUniformData(uniforms, sizeof(uniforms));
    panoramaUniformsDirty_ = false;
}

void VideoRenderer::update(float deltaTime) {
    if (renderMode_ == RenderMode::Panorama) {
        updatePanoramaUniforms();
    }
}
