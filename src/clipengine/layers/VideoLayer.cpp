#include "VideoLayer.h"
#include "../shaders/ShaderLibrary.h"

#if _WIN32
#include <dawn/native/D3D11Backend.h>
#include <dawn/native/D3D12Backend.h>
#elif __APPLE__
#endif
#include <iostream>

// stb_image for loading images in loadFromFile
#define STB_IMAGE_IMPLEMENTATION
#include "../utils/stb_image.h"

VideoLayer::VideoLayer()
    : ImageLayer(ShaderPresets::createNV12VideoShader())
    , videoFormat_(VideoFormat::NV12) {
}

VideoLayer::VideoLayer(const ShaderConfig& config)
    : ImageLayer(config) {
}

VideoLayer::VideoLayer(VideoFormat format)
    : ImageLayer(ShaderPresets::createNV12VideoShader())  // Temporary, will be updated
    , videoFormat_(format) {
    createShaderForMode();
}

VideoLayer::~VideoLayer() {
    cleanupSharedTextures();
}

void VideoLayer::cleanupSharedTextures() {
    // End access to Dawn texture if active
    if (dawnTextureData_.sharedMemory && dawnTextureData_.texture) {
        wgpu::SharedTextureMemoryEndAccessState endState = {};
        dawnTextureData_.sharedMemory.EndAccess(dawnTextureData_.texture, &endState);
    }

    // Release Dawn resources
    dawnTextureData_.texture = nullptr;
    dawnTextureData_.sharedMemory = nullptr;
    dawnTextureData_.width = 0;
    dawnTextureData_.height = 0;

    // Release texture views
    yPlaneView_ = nullptr;
    uvPlaneView_ = nullptr;
    uPlaneView_ = nullptr;
    vPlaneView_ = nullptr;

    // Close shared handle and release D3D11 texture
    if (sharedTextureData_.handle) {
        CloseHandle(sharedTextureData_.handle);
        sharedTextureData_.handle = nullptr;
    }
    sharedTextureData_.texture = nullptr;
    sharedTextureData_.width = 0;
    sharedTextureData_.height = 0;
}

void VideoLayer::setVideoFormat(VideoFormat format) {
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
    bool CreateD3D11SharedTexture(ComPtr<ID3D11Device>& device, const D3D11_TEXTURE2D_DESC& srcDesc, VideoLayer::SharedTextureData& outData) {
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

    VideoLayer::DawnTextureData ImportToDawnTexture(wgpu::Device& device, HANDLE sharedHandle) {
        VideoLayer::DawnTextureData result;

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

bool VideoLayer::updateFrame(ID3D11Texture2D* texture, int arrayIndex) {
    if(!texture) return false;

    ComPtr<ID3D11Device> d3d11Device;
    texture->GetDevice(d3d11Device.GetAddressOf());
    ComPtr<ID3D11DeviceContext> ctx;
    d3d11Device->GetImmediateContext(ctx.GetAddressOf());

    D3D11_TEXTURE2D_DESC srcDesc = {};
    texture->GetDesc(&srcDesc);

    // Check if resolution changed - if so, cleanup old textures
    bool resolutionChanged = (sharedTextureData_.width != srcDesc.Width ||
                              sharedTextureData_.height != srcDesc.Height);

    if (!sharedTextureData_.texture || resolutionChanged) {
        if (resolutionChanged) {
            std::cout << "[VideoLayer] Resolution changed: "
                      << sharedTextureData_.width << "x" << sharedTextureData_.height
                      << " -> " << srcDesc.Width << "x" << srcDesc.Height << std::endl;
            cleanupSharedTextures();
        }

        if (!CreateD3D11SharedTexture(d3d11Device, srcDesc, sharedTextureData_)) {
            return false;
        }
    }

    ctx->CopySubresourceRegion(sharedTextureData_.texture.Get(), 0, 0, 0, 0, texture, arrayIndex, nullptr);
    ctx->Flush();

    // Recreate Dawn texture if resolution changed or not yet created
    if (!dawnTextureData_.texture || resolutionChanged) {
        // End access to old texture if it exists
        if (dawnTextureData_.sharedMemory && dawnTextureData_.texture) {
            wgpu::SharedTextureMemoryEndAccessState endState = {};
            dawnTextureData_.sharedMemory.EndAccess(dawnTextureData_.texture, &endState);
        }

        dawnTextureData_ = ImportToDawnTexture(device_, sharedTextureData_.handle);
        if (!dawnTextureData_.texture) return false;

        dawnTextureData_.width = srcDesc.Width;
        dawnTextureData_.height = srcDesc.Height;

        // Clear cached texture views - they will be recreated below
        yPlaneView_ = nullptr;
        uvPlaneView_ = nullptr;
        uPlaneView_ = nullptr;
        vPlaneView_ = nullptr;
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
            yPlaneView_ = dawnTextureData_.texture.CreateView(&yViewDesc);

            wgpu::TextureViewDescriptor uvViewDesc = {
                .format          = wgpu::TextureFormat::RG8Unorm,
                .dimension       = wgpu::TextureViewDimension::e2D,
                .baseMipLevel    = 0,
                .mipLevelCount   = 1,
                .baseArrayLayer  = 0,
                .arrayLayerCount = 1,
                .aspect          = wgpu::TextureAspect::Plane1Only
            };
            uvPlaneView_ = dawnTextureData_.texture.CreateView(&uvViewDesc);

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
            yPlaneView_ = dawnTextureData_.texture.CreateView(&rgbaViewDesc);
            views = {yPlaneView_};
            break;
        }
    }

    // Update textures using base class method
    updateTextures(views);

    return true;
}

bool VideoLayer::loadFromFile(const std::string& filename) {
    // Load image using stb_image
    int width, height, channels;
    unsigned char* imageData = stbi_load(filename.c_str(), &width, &height, &channels, 4);  // Force RGBA

    if (!imageData) {
        std::cerr << "[VideoLayer] Failed to load image: " << filename << std::endl;
        std::cerr << "[VideoLayer] Reason: " << stbi_failure_reason() << std::endl;
        return false;
    }

    std::cout << "[VideoLayer] Loaded image: " << filename << std::endl;
    std::cout << "[VideoLayer]   Size: " << width << "x" << height << std::endl;
    std::cout << "[VideoLayer]   Channels: " << channels << " (converted to RGBA)" << std::endl;

    // Create D3D11 device for texture creation
    ComPtr<ID3D11Device> d3d11Device;
    ComPtr<ID3D11DeviceContext> d3d11Context;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    HRESULT hr = D3D11CreateDevice(
        nullptr,                    // Use default adapter
        D3D_DRIVER_TYPE_HARDWARE,   // Hardware device
        nullptr,                    // No software rasterizer
        0,                          // No special flags
        featureLevels,
        _countof(featureLevels),
        D3D11_SDK_VERSION,
        d3d11Device.GetAddressOf(),
        nullptr,
        d3d11Context.GetAddressOf()
    );

    if (FAILED(hr)) {
        std::cerr << "[VideoLayer] Failed to create D3D11 device for image loading, HRESULT: 0x"
                  << std::hex << hr << std::dec << std::endl;
        stbi_image_free(imageData);
        return false;
    }

    // Create D3D11 texture
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = imageData;
    initData.SysMemPitch = width * 4;  // RGBA = 4 bytes per pixel
    initData.SysMemSlicePitch = 0;

    ComPtr<ID3D11Texture2D> texture;
    hr = d3d11Device->CreateTexture2D(&texDesc, &initData, texture.GetAddressOf());

    // Free image data
    stbi_image_free(imageData);

    if (FAILED(hr)) {
        std::cerr << "[VideoLayer] Failed to create D3D11 texture, HRESULT: 0x"
                  << std::hex << hr << std::dec << std::endl;
        return false;
    }

    std::cout << "[VideoLayer] Image texture created successfully" << std::endl;

    // Use updateFrame to upload the texture
    return updateFrame(texture.Get(), 0);
}

// ============================================================================
// Panorama Mode Support
// ============================================================================

void VideoLayer::createShaderForMode() {
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
    // Little Planet mode - stereographic projection
    else if (renderMode_ == RenderMode::LittlePlanet) {
        switch (videoFormat_) {
            case VideoFormat::NV12:
                shaderConfig_ = ShaderLibrary::create(ShaderType::LittlePlanetNV12);
                break;
            case VideoFormat::RGBA:
                shaderConfig_ = ShaderLibrary::create(ShaderType::LittlePlanetRGBA);
                break;
            case VideoFormat::I420:
                // TODO: Implement I420 little planet shader, use NV12 as fallback
                shaderConfig_ = ShaderLibrary::create(ShaderType::LittlePlanetNV12);
                break;
        }
    }
    // Crystal Ball mode - inverse stereographic/fisheye
    else if (renderMode_ == RenderMode::CrystalBall) {
        switch (videoFormat_) {
            case VideoFormat::NV12:
                shaderConfig_ = ShaderLibrary::create(ShaderType::CrystalBallNV12);
                break;
            case VideoFormat::RGBA:
                shaderConfig_ = ShaderLibrary::create(ShaderType::CrystalBallRGBA);
                break;
            case VideoFormat::I420:
                // TODO: Implement I420 crystal ball shader, use NV12 as fallback
                shaderConfig_ = ShaderLibrary::create(ShaderType::CrystalBallNV12);
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

void VideoLayer::setRenderMode(RenderMode mode) {
    if (renderMode_ == mode) return;

    // Instead of rebuilding immediately, defer it to the next update() call
    // This ensures we don't invalidate the pipeline while GPU is still using it
    pendingRenderMode_ = mode;
    pipelineNeedsRebuild_ = true;

    std::cout << "[VideoLayer] Mode switch requested: " << (int)renderMode_
              << " -> " << (int)mode << std::endl;
}

void VideoLayer::setRotation(float yawRadians, float pitchRadians) {
    panoramaParams_.yaw = yawRadians;
    panoramaParams_.pitch = pitchRadians;
    panoramaUniformsDirty_ = true;
}

void VideoLayer::setZoom(float zoom) {
    panoramaParams_.zoom = zoom;
    panoramaUniformsDirty_ = true;
}

void VideoLayer::setAspect(float aspect) {
    panoramaParams_.aspect = aspect;
    panoramaUniformsDirty_ = true;
}

void VideoLayer::updatePanoramaUniforms() {
    if (!panoramaUniformsDirty_) return;

    // Pack panorama parameters into uniform buffer
    // Order must match shader uniform: yaw, pitch, zoom, aspect
    float uniforms[4] = {
        panoramaParams_.yaw,
        panoramaParams_.pitch,
        panoramaParams_.zoom,
        panoramaParams_.aspect
    };

    updateUniformData(uniforms, sizeof(uniforms));
    panoramaUniformsDirty_ = false;
}

void VideoLayer::update(float deltaTime) {
    // Handle deferred pipeline rebuild at the start of the frame
    // This ensures we don't invalidate the pipeline while GPU is still using it
    if (pipelineNeedsRebuild_ && device_) {
        std::cout << "[VideoLayer] Rebuilding pipeline for mode: " << (int)pendingRenderMode_ << std::endl;
        pipelineNeedsRebuild_ = false;
        renderMode_ = pendingRenderMode_;

        // Recreate shader and pipeline for the new mode
        createShaderForMode();
        initializeShader();
        initializePipeline();
        std::cout << "[VideoLayer] Pipeline rebuild complete" << std::endl;

        // For panorama/little planet/crystal ball modes, setup uniform buffer
        if (renderMode_ == RenderMode::Panorama ||
            renderMode_ == RenderMode::LittlePlanet ||
            renderMode_ == RenderMode::CrystalBall) {
            // Prepare uniform data
            // Order must match shader uniform: yaw, pitch, zoom, aspect
            float uniforms[4] = {
                panoramaParams_.yaw,
                panoramaParams_.pitch,
                panoramaParams_.zoom,
                panoramaParams_.aspect
            };

            // Create or update uniform buffer
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

        // Rebuild bindGroup with all resources ready
        if (!textureViews_.empty()) {
            updateBindGroup();
        }
    }

    // Update panorama uniforms if they changed (rotation, zoom, etc.)
    if (renderMode_ == RenderMode::Panorama ||
        renderMode_ == RenderMode::LittlePlanet ||
        renderMode_ == RenderMode::CrystalBall) {
        updatePanoramaUniforms();
    }
}
