#include "VideoRenderer.h"
// #include "../util/GPUProfiler.h"  // Temporarily disabled due to compilation issues
#include <dawn/native/D3D11Backend.h>
#include <dawn/native/D3D12Backend.h>
#include <iostream>

VideoRenderer::VideoRenderer() = default;
VideoRenderer::~VideoRenderer() = default;

bool VideoRenderer::initialize(wgpu::Device device, wgpu::TextureFormat format) {
    device_ = device;
    surfaceFormat_ = format;

    initializeBuffers();
    initializeSampler();
    initializeShader();
    initializePipeline();

    return true;
}

void VideoRenderer::initializeBuffers() {
    wgpu::BufferDescriptor bufferDesc = {};
    bufferDesc.size = sizeof(float) * 4 * 6;  // 6 vertices, 4 floats each (pos + uv)
    bufferDesc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    vertexBuffer_ = device_.CreateBuffer(&bufferDesc);

    updateVertexBuffer();
}

void VideoRenderer::initializeSampler() {
    wgpu::SamplerDescriptor samplerDesc = {};
    samplerDesc.addressModeU = wgpu::AddressMode::ClampToEdge;
    samplerDesc.addressModeV = wgpu::AddressMode::ClampToEdge;
    samplerDesc.addressModeW = wgpu::AddressMode::ClampToEdge;
    samplerDesc.magFilter = wgpu::FilterMode::Linear;
    samplerDesc.minFilter = wgpu::FilterMode::Linear;
    samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
    sampler_ = device_.CreateSampler(&samplerDesc);
}

void VideoRenderer::initializeShader() {
    const char* shaderSource = R"(
        @group(0) @binding(0) var mySampler : sampler;
        @group(0) @binding(1) var yTex : texture_2d<f32>;
        @group(0) @binding(2) var uvTex : texture_2d<f32>;

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

    wgpu::ShaderModuleWGSLDescriptor wgslDesc = {};
    wgslDesc.code = shaderSource;

    wgpu::ShaderModuleDescriptor moduleDesc = {};
    moduleDesc.nextInChain = &wgslDesc;
    shaderModule_ = device_.CreateShaderModule(&moduleDesc);
}

void VideoRenderer::initializePipeline() {
    // Bind group layout
    wgpu::BindGroupLayoutEntry entries[3] = {};
    entries[0].binding = 0;
    entries[0].visibility = wgpu::ShaderStage::Fragment;
    entries[0].sampler.type = wgpu::SamplerBindingType::Filtering;

    entries[1].binding = 1;
    entries[1].visibility = wgpu::ShaderStage::Fragment;
    entries[1].texture.sampleType = wgpu::TextureSampleType::Float;
    entries[1].texture.viewDimension = wgpu::TextureViewDimension::e2D;

    entries[2].binding = 2;
    entries[2].visibility = wgpu::ShaderStage::Fragment;
    entries[2].texture.sampleType = wgpu::TextureSampleType::Float;
    entries[2].texture.viewDimension = wgpu::TextureViewDimension::e2D;

    wgpu::BindGroupLayoutDescriptor bglDesc = {};
    bglDesc.entryCount = 3;
    bglDesc.entries = entries;
    bindGroupLayout_ = device_.CreateBindGroupLayout(&bglDesc);

    // Pipeline layout
    wgpu::PipelineLayoutDescriptor layoutDesc = {};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = &bindGroupLayout_;
    wgpu::PipelineLayout pipelineLayout = device_.CreatePipelineLayout(&layoutDesc);

    // Vertex state
    wgpu::VertexAttribute attrs[2] = {};
    attrs[0].format = wgpu::VertexFormat::Float32x2;
    attrs[0].offset = 0;
    attrs[0].shaderLocation = 0;
    attrs[1].format = wgpu::VertexFormat::Float32x2;
    attrs[1].offset = sizeof(float) * 2;
    attrs[1].shaderLocation = 1;

    wgpu::VertexBufferLayout vbLayout = {};
    vbLayout.arrayStride = sizeof(float) * 4;
    vbLayout.attributeCount = 2;
    vbLayout.attributes = attrs;

    // Fragment state
    wgpu::ColorTargetState colorTarget = {};
    colorTarget.format = surfaceFormat_;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;

    wgpu::FragmentState fragmentState = {};
    fragmentState.module = shaderModule_;
    fragmentState.entryPoint = "fs";
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    // Pipeline
    wgpu::RenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.layout = pipelineLayout;
    pipelineDesc.vertex.module = shaderModule_;
    pipelineDesc.vertex.entryPoint = "vs";
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vbLayout;
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;

    pipeline_ = device_.CreateRenderPipeline(&pipelineDesc);
}

namespace {
    struct SharedTextureData {
        ComPtr<ID3D11Texture2D> texture;
        HANDLE handle = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    bool CreateD3D11SharedTexture(ComPtr<ID3D11Device>& device, const D3D11_TEXTURE2D_DESC& srcDesc, SharedTextureData& outData) {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = srcDesc.Width;
        desc.Height = srcDesc.Height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = (srcDesc.Format == DXGI_FORMAT_NV12 || srcDesc.Format == 103) ? DXGI_FORMAT_NV12 : srcDesc.Format;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE;

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

        wgpu::TextureDescriptor texDesc = {};
        texDesc.usage = props.usage;
        texDesc.dimension = wgpu::TextureDimension::e2D;
        texDesc.size = props.size;
        texDesc.format = props.format;
        texDesc.mipLevelCount = 1;
        texDesc.sampleCount = 1;

        result.texture = result.sharedMemory.CreateTexture(&texDesc);

        wgpu::SharedTextureMemoryBeginAccessDescriptor beginDesc = {};
        beginDesc.initialized = true;
        result.sharedMemory.BeginAccess(result.texture, &beginDesc);

        return result;
    }
}

bool VideoRenderer::updateFrame(ID3D11Texture2D* texture, int arrayIndex) {
    if(!texture) return false;

    // 性能测量：GPU 拷贝操作
    // if (profiler_) profiler_->beginEvent("GPU_Copy");  // Temporarily disabled

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

    wgpu::TextureViewDescriptor yViewDesc = {};
    yViewDesc.format = wgpu::TextureFormat::R8Unorm;
    yViewDesc.dimension = wgpu::TextureViewDimension::e2D;
    yViewDesc.baseMipLevel = 0;
    yViewDesc.mipLevelCount = 1;
    yViewDesc.baseArrayLayer = 0;
    yViewDesc.arrayLayerCount = 1;
    yViewDesc.aspect = wgpu::TextureAspect::Plane0Only;
    yPlaneView_ = dawnData.texture.CreateView(&yViewDesc);

    wgpu::TextureViewDescriptor uvViewDesc = {};
    uvViewDesc.format = wgpu::TextureFormat::RG8Unorm;
    uvViewDesc.dimension = wgpu::TextureViewDimension::e2D;
    uvViewDesc.baseMipLevel = 0;
    uvViewDesc.mipLevelCount = 1;
    uvViewDesc.baseArrayLayer = 0;
    uvViewDesc.arrayLayerCount = 1;
    uvViewDesc.aspect = wgpu::TextureAspect::Plane1Only;
    uvPlaneView_ = dawnData.texture.CreateView(&uvViewDesc);

    updateBindGroup();

    // if (profiler_) profiler_->endEvent();  // End GPU_Copy  // Temporarily disabled

    return true;
}


void VideoRenderer::updateBindGroup() {
    wgpu::BindGroupEntry entries[3] = {};
    entries[0].binding = 0;
    entries[0].sampler = sampler_;
    entries[1].binding = 1;
    entries[1].textureView = yPlaneView_;
    entries[2].binding = 2;
    entries[2].textureView = uvPlaneView_;

    wgpu::BindGroupDescriptor bgDesc = {};
    bgDesc.layout = bindGroupLayout_;
    bgDesc.entryCount = 3;
    bgDesc.entries = entries;
    bindGroup_ = device_.CreateBindGroup(&bgDesc);
}

void VideoRenderer::render(wgpu::RenderPassEncoder& pass) {
    if(!enabled_ || !bindGroup_) return;

    // 性能测量：渲染操作
    // if (profiler_) profiler_->beginEvent("Render");  // Temporarily disabled

    pass.SetPipeline(pipeline_);
    pass.SetVertexBuffer(0, vertexBuffer_);
    pass.SetBindGroup(0, bindGroup_);
    pass.Draw(6);

    // if (profiler_) profiler_->endEvent();  // End Render  // Temporarily disabled
}

void VideoRenderer::update(float deltaTime) {
}

void VideoRenderer::updateVertexBuffer() {
    // Convert viewport (0-1 normalized) to NDC (-1 to 1)
    float x1 = viewport_.x * 2.0f - 1.0f;
    float y1 = viewport_.y * 2.0f - 1.0f;
    float x2 = (viewport_.x + viewport_.w) * 2.0f - 1.0f;
    float y2 = (viewport_.y + viewport_.h) * 2.0f - 1.0f;

    float vertices[] = {
        // pos.x, pos.y, uv.x, uv.y
        x1, y1, 0.0f, 1.0f,  // bottom-left
        x2, y1, 1.0f, 1.0f,  // bottom-right
        x1, y2, 0.0f, 0.0f,  // top-left
        x1, y2, 0.0f, 0.0f,  // top-left
        x2, y1, 1.0f, 1.0f,  // bottom-right
        x2, y2, 1.0f, 0.0f   // top-right
    };

    device_.GetQueue().WriteBuffer(vertexBuffer_, 0, vertices, sizeof(vertices));
}

void VideoRenderer::setViewport(float x, float y, float width, float height) {
    viewport_.x = x;
    viewport_.y = y;
    viewport_.w = width;
    viewport_.h = height;

    if (vertexBuffer_) {
        updateVertexBuffer();
    }
}
