#include "VideoRenderer.h"
#include "../shaders/ShaderLibrary.h"

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
    createShaderForMode();
}

VideoRenderer::~VideoRenderer() = default;

void VideoRenderer::setVideoFormat(VideoFormat format) {
    if (videoFormat_ == format) return;

    videoFormat_ = format;
    createShaderForMode();

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

void VideoRenderer::createShaderForMode() {
    // Panorama mode - 360° spherical rendering
    if (renderMode_ == RenderMode::Panorama) {
        switch (videoFormat_) {
            case VideoFormat::NV12:
                shaderConfig_ = ShaderLibrary::create(ShaderType::PanoramaNV12);
                break;
            case VideoFormat::RGBA:
                shaderConfig_ = ShaderLibrary::create(ShaderType::PanoramaRGBA);
                break;
            case VideoFormat::I420:
                // TODO: Implement I420 panorama shader, use NV12 as fallback
                shaderConfig_ = ShaderLibrary::create(ShaderType::PanoramaNV12);
                break;
        }
    }
    // Planar mode - standard 2D rendering
    else {
        switch (videoFormat_) {
            case VideoFormat::NV12:
                shaderConfig_ = ShaderLibrary::create(ShaderType::PlanarNV12);
                break;
            case VideoFormat::RGBA:
                shaderConfig_ = ShaderLibrary::create(ShaderType::PlanarRGBA);
                break;
            case VideoFormat::I420:
                // Use legacy inline shader for I420 planar (TODO: move to external file)
                shaderConfig_ = ShaderPresets::createI420VideoShader();
                break;
        }
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

        // IMPORTANT: For panorama mode, we need uniform buffer with data before creating bindGroup
        // But we must not call updatePanoramaUniforms() because it calls updateBindGroup() internally
        // Instead, directly prepare the uniform data
        if (renderMode_ == RenderMode::Panorama) {
            // Prepare uniform data without calling updateBindGroup
            float uniforms[4] = {
                panoramaParams_.yaw,
                panoramaParams_.pitch,
                panoramaParams_.zoom,
                panoramaParams_.aspect
            };

            // Create or update uniform buffer manually
            if (!uniformBuffer_ || uniformBufferSize_ < sizeof(uniforms)) {
                uniformBufferSize_ = sizeof(uniforms);
                wgpu::BufferDescriptor bufDesc = {};
                bufDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
                bufDesc.size = uniformBufferSize_;
                uniformBuffer_ = device_.CreateBuffer(&bufDesc);
            }
            device_.GetQueue().WriteBuffer(uniformBuffer_, 0, uniforms, sizeof(uniforms));
            panoramaUniformsDirty_ = false;
        }

        // Now rebuild bindGroup with all resources ready
        if (!textureViews_.empty()) {
            updateBindGroup();
        }
    }
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
