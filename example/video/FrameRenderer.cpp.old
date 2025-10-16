#include "FrameRenderer.h"
#include <iostream>

using namespace wgpu;

void FrameRenderer::init() {}

void FrameRenderer::init_buffer()
{
    float vertex_data[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 0.0f
    };
    vertex_buffer_ = dawn::utils::CreateBufferFromData(device_, vertex_data, sizeof(vertex_data), BufferUsage::Vertex);
}

void FrameRenderer::init_sampler()
{
    sampler_ = dawn::utils::CreateSamper(device_);
}

void FrameRenderer::init_shader()
{
    module_ = dawn::utils::CreateShaderModuleFromePath(device_, "D:/TestDawn/src/core/shader/nv12_renderer.wgsl");
}

void FrameRenderer::init_bindgroup()
{
    if (!sampler_) init_sampler();

    BindGroupEntry entries[3] = {
        {.binding = 0, .sampler = sampler_},
        {.binding = 1, .textureView = y_tex_},
        {.binding = 2, .textureView = uv_tex_}
    };

    BindGroupDescriptor desc = {
        .layout = pipeline_.GetBindGroupLayout(0),
        .entryCount = 3,
        .entries = entries
    };
    bind_group_ = device_.CreateBindGroup(&desc);
}

namespace {
    struct SharedTextureData {
        ComPtr<ID3D11Texture2D> texture;
        HANDLE handle = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    bool CreateD3D11SharedTexture(ComPtr<ID3D11Device>& device, const D3D11_TEXTURE2D_DESC& srcDesc, SharedTextureData& outData)
    {
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

    DawnTextureData ImportToDawnTexture(wgpu::Device& device, HANDLE sharedHandle)
    {
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

void FrameRenderer::update_texture(ID3D11Texture2D* tex, int index)
{
    ComPtr<ID3D11Device> d3d11Device;
    tex->GetDevice(d3d11Device.GetAddressOf());
    ComPtr<ID3D11DeviceContext> ctx;
    d3d11Device->GetImmediateContext(ctx.GetAddressOf());

    D3D11_TEXTURE2D_DESC srcDesc = {};
    tex->GetDesc(&srcDesc);

    static SharedTextureData sharedData;

    if(!sharedData.texture || sharedData.width != srcDesc.Width || sharedData.height != srcDesc.Height) {
        if(sharedData.handle) CloseHandle(sharedData.handle);
        sharedData = {};
        if(!CreateD3D11SharedTexture(d3d11Device, srcDesc, sharedData)) return;
    }

    ctx->CopySubresourceRegion(sharedData.texture.Get(), 0, 0, 0, 0, tex, index, nullptr);
    ctx->Flush();

    static DawnTextureData dawnData;
    static uint32_t lastWidth = 0, lastHeight = 0;

    if(!dawnData.texture || lastWidth != srcDesc.Width || lastHeight != srcDesc.Height) {
        dawnData = ImportToDawnTexture(device_, sharedData.handle);
        if(!dawnData.texture) return;
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
    y_tex_ = dawnData.texture.CreateView(&yViewDesc);

    wgpu::TextureViewDescriptor uvViewDesc = {};
    uvViewDesc.format = wgpu::TextureFormat::RG8Unorm;
    uvViewDesc.dimension = wgpu::TextureViewDimension::e2D;
    uvViewDesc.baseMipLevel = 0;
    uvViewDesc.mipLevelCount = 1;
    uvViewDesc.baseArrayLayer = 0;
    uvViewDesc.arrayLayerCount = 1;
    uvViewDesc.aspect = wgpu::TextureAspect::Plane1Only;
    uv_tex_ = dawnData.texture.CreateView(&uvViewDesc);
}

void FrameRenderer::init_pipeline()
{
    if (!module_) init_shader();

    VertexAttribute attrs[] = {
        {.format = VertexFormat::Float32x2, .offset = 0, .shaderLocation = 0},
        {.format = VertexFormat::Float32x2, .offset = sizeof(float) * 2, .shaderLocation = 1}
    };
    VertexBufferLayout vb_layout = {.arrayStride = sizeof(float) * 4, .attributeCount = 2, .attributes = attrs};

    ColorTargetState target = {.format = surface_texture_fmt_, .writeMask = ColorWriteMask::All};
    FragmentState fragment_state = {.module = module_, .entryPoint = "fs", .targetCount = 1, .targets = &target};

    bind_group_layout_ = dawn::utils::MakeBindGroupLayout(device_, {
        {0, ShaderStage::Fragment, SamplerBindingType::Filtering},
        {1, ShaderStage::Fragment, TextureSampleType::Float, TextureViewDimension::e2D},
        {2, ShaderStage::Fragment, TextureSampleType::Float, TextureViewDimension::e2D}
    });

    RenderPipelineDescriptor desc = {
        .layout = dawn::utils::MakeBasicPipelineLayout(device_, &bind_group_layout_),
        .vertex = {.module = module_, .entryPoint = "vs", .bufferCount = 1, .buffers = &vb_layout},
        .fragment = &fragment_state
    };
    pipeline_ = device_.CreateRenderPipeline(&desc);
}

void FrameRenderer::render(RenderPassEncoder& pass)
{
    if(!pipeline_) init_pipeline();
    if(!vertex_buffer_) init_buffer();
    if(!bind_group_) init_bindgroup();

    pass.SetPipeline(pipeline_);
    pass.SetVertexBuffer(0, vertex_buffer_);
    pass.SetBindGroup(0, bind_group_);
    pass.Draw(6);
}
