#include "FrameRenderer.h"
#include <iterator>
#include <fstream>
#include <vector>
#include <iostream>

using namespace wgpu;

uint32_t width = 5888;
uint32_t height= 3840;


void FrameRenderer::init()
{
    // 不再加载测试纹理，直接使用 update_texture() 从硬解码获取视频帧
    // init_texture();  // 已禁用
}

void FrameRenderer::init_buffer()
{
    float vertex_data[] = {
        -1.0f, -1.0f, 0.0f, 1.0f, // 左下
         1.0f, -1.0f, 1.0f, 1.0f, // 右下
        -1.0f,  1.0f, 0.0f, 0.0f, // 左上

        -1.0f,  1.0f, 0.0f, 0.0f, // 左上
         1.0f, -1.0f, 1.0f, 1.0f, // 右下
         1.0f,  1.0f, 1.0f, 0.0f  // 右上
    };
    vertex_buffer_ = dawn::utils::CreateBufferFromData(
        device_, vertex_data, sizeof(vertex_data), BufferUsage::Vertex
    );
}

void FrameRenderer::init_sampler()
{
    sampler_ = dawn::utils::CreateSamper(device_);
}
int k_w = 7680;
int k_h = 3840;

void FrameRenderer::init_8k_texture()
{
    
}

void FrameRenderer::init_texture()
{
    // 从 YUV 文件加载测试数据 (用于开发测试)
    // 实际使用时通过 update_texture() 从硬解码获取 DX11 纹理
    const char* path = "D://5888x3840.yuv";
    // 加载 yuv 数据
    

    int y_size = width * height;
    int u_size = width * height / 4;
    int v_size = u_size;

    std::ifstream file(path, std::ios::binary);
    if(!file) throw std::runtime_error("Cannot open yuv file");

    std::vector<uint8_t> y;
    std::vector<uint8_t> u;
    std::vector<uint8_t> v;

    y.resize(y_size);
    u.resize(u_size);
    v.resize(v_size);

    file.read((char*)y.data(), y_size);
    file.read((char*)u.data(), u_size);
    file.read((char*)v.data(), v_size);

    wgpu::TextureDescriptor tex_desc {
        .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst,
        .dimension = TextureDimension::e2D,
        .format = TextureFormat::R8Unorm,
        .mipLevelCount = 1,
        .sampleCount = 1,
    };

    tex_desc.size = {
        .width = width,
        .height= height,
        .depthOrArrayLayers = 1
    };
    wgpu::Texture tex_y = device_.CreateTexture(&tex_desc);
    TexelCopyTextureInfo info;
    info.texture = tex_y;
    info.mipLevel = 0;
    info.origin = {0, 0, 0};

    TexelCopyBufferLayout layout;
    layout.offset = 0;
    layout.bytesPerRow = width;
    layout.rowsPerImage = height;

    Extent3D copySize{};
    copySize.width = width;
    copySize.height = height;
    copySize.depthOrArrayLayers = 1;

    device_.GetQueue().WriteTexture(&info, (void*)y.data(), y_size, &layout, &copySize);

    tex_desc.size = {
        .width = width / 2,
        .height= height / 2,
        .depthOrArrayLayers = 1
    };
    wgpu::Texture tex_u = device_.CreateTexture(&tex_desc);
    info.texture = tex_u;
    layout.offset = 0;
    layout.bytesPerRow = width / 2;
    layout.rowsPerImage = height / 2;
    copySize.width = width / 2;
    copySize.height= height / 2;
    device_.GetQueue().WriteTexture(&info, (void*)u.data(), u_size, &layout, &copySize);

    wgpu::Texture tex_v = device_.CreateTexture(&tex_desc);
    info.texture = tex_v;
    layout.offset = 0;
    layout.bytesPerRow = width / 2;
    layout.rowsPerImage = height / 2;
    copySize.width = width / 2;
    copySize.height= height / 2;
    device_.GetQueue().WriteTexture(&info, (void*)v.data(), u_size, &layout, &copySize);

    y_tex_ = tex_y.CreateView();
    u_tex_ = tex_u.CreateView();
    v_tex_ = tex_v.CreateView();
}

void FrameRenderer::init_shader()
{
    module_ = dawn::utils::CreateShaderModuleFromePath(device_, "D:/TestDawn/src/core/shader/y_plane_renderer.wgsl");
}

void FrameRenderer::init_bindgroup()
{
    // 渲染管线直接使用 Y 纹理
    if (!sampler_) {
        init_sampler();
    }

    // 简化的 bindgroup，只需要 sampler 和 Y 纹理
    BindGroupEntry entries[2]{};
    entries[0].binding = 0;
    entries[0].sampler = sampler_;
    entries[1].binding = 1;
    entries[1].textureView = y_tex_;

    BindGroupDescriptor renderBGDesc{
        .layout = pipeline_.GetBindGroupLayout(0),
        .entryCount = 2,
        .entries = entries
    };
    bind_group_ = device_.CreateBindGroup(&renderBGDesc);
}

void FrameRenderer::update_texture(ID3D11Texture2D* tex, int index)
{
    // ===============================
    // 硬解码纹理转换流程：
    // DX11 Texture (硬解码) -> DX11 共享纹理 -> DX12 Resource -> Dawn Texture
    // ===============================

    // -------------------------------
    // 1️⃣ 获取 D3D11 设备和上下文
    // -------------------------------
    ComPtr<ID3D11Device> d3d11Device;
    tex->GetDevice(d3d11Device.GetAddressOf());
    ComPtr<ID3D11DeviceContext> ctx;
    d3d11Device->GetImmediateContext(ctx.GetAddressOf());

    // 获取源纹理描述
    D3D11_TEXTURE2D_DESC srcDesc = {};
    tex->GetDesc(&srcDesc);

    // 重要：硬解码器通常输出纹理数组，index 参数指定数组索引
    // 但创建共享纹理时我们只需要单个纹理，不需要数组
    // 所以我们复制特定的数组切片到非数组纹理

    // -------------------------------
    // 2️⃣ 创建或复用 D3D11 共享纹理
    // -------------------------------
    static ComPtr<ID3D11Texture2D> sharedTex;
    static HANDLE sharedHandle = nullptr;
    static uint32_t lastWidth = 0;
    static uint32_t lastHeight = 0;

    // 如果纹理尺寸变化，重新创建
    if(!sharedTex || lastWidth != srcDesc.Width || lastHeight != srcDesc.Height) {
        sharedTex.Reset();
        if(sharedHandle) {
            CloseHandle(sharedHandle);
            sharedHandle = nullptr;
        }

        // 先输出源纹理信息用于调试
        std::cout << "Source texture: " << std::hex << srcDesc.Width << "x" << srcDesc.Height << std::dec
                  << " format: " << srcDesc.Format
                  << " ArraySize: " << srcDesc.ArraySize
                  << " BindFlags: " << srcDesc.BindFlags
                  << " MiscFlags: " << srcDesc.MiscFlags << std::endl;

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = srcDesc.Width;
        desc.Height = srcDesc.Height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;  // 重要：共享纹理必须是单个纹理，不能是数组
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;

        // 根据格式选择合适的配置
        // 格式 103 = DXGI_FORMAT_NV12 (视频格式)
        // 格式 67  = DXGI_FORMAT_R8_UNORM (单通道 8 位)
        if(srcDesc.Format == DXGI_FORMAT_NV12 || srcDesc.Format == 103) {
            // NV12 格式：视频格式，用于硬解码输出
            desc.Format = DXGI_FORMAT_NV12;
            desc.BindFlags = 0;  // NV12 不支持 BIND_SHADER_RESOURCE
            desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
        } else if(srcDesc.Format == DXGI_FORMAT_R8_UNORM || srcDesc.Format == 67) {
            // R8_UNORM 格式：单通道格式（可能是 Y 平面或其他单通道数据）
            desc.Format = DXGI_FORMAT_R8_UNORM;
            desc.BindFlags = 0;  // 共享纹理通常不需要 BindFlags
            desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
        } else {
            // 其他格式
            desc.Format = srcDesc.Format;
            desc.BindFlags = 0;
            desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
        }

        HRESULT hr = d3d11Device->CreateTexture2D(&desc, nullptr, sharedTex.GetAddressOf());
        if(FAILED(hr)) {
            std::cout << "Create shared texture failed: " << std::hex << hr << std::endl;
            std::cout << "Attempted: " << std::dec << desc.Width << "x" << desc.Height
                      << " format: " << desc.Format
                      << " BindFlags: " << desc.BindFlags
                      << " MiscFlags: " << desc.MiscFlags << std::endl;

            // 尝试去掉 SHARED_NTHANDLE，使用普通 SHARED
            desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
            hr = d3d11Device->CreateTexture2D(&desc, nullptr, sharedTex.GetAddressOf());
            if(FAILED(hr)) {
                std::cout << "Retry with MISC_SHARED also failed: " << std::hex << hr << std::endl;
                return;
            }
            std::cout << "Success with D3D11_RESOURCE_MISC_SHARED (non-NT handle)" << std::endl;
        }

        // 获取共享句柄
        // 检查实际创建的纹理使用的 MiscFlags
        D3D11_TEXTURE2D_DESC createdDesc;
        sharedTex->GetDesc(&createdDesc);
        bool isNTHandle = (createdDesc.MiscFlags & D3D11_RESOURCE_MISC_SHARED_NTHANDLE) != 0;

        if(isNTHandle) {
            // 使用 NT Handle (推荐方式，支持跨进程)
            ComPtr<IDXGIResource1> dxgiRes;
            hr = sharedTex.As(&dxgiRes);
            if(FAILED(hr)) {
                std::cout << "Query IDXGIResource1 failed: " << std::hex << hr << std::endl;
                return;
            }

            hr = dxgiRes->CreateSharedHandle(
                nullptr,
                DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE,
                nullptr,
                &sharedHandle
            );
            if(FAILED(hr)) {
                std::cout << "CreateSharedHandle failed: " << std::hex << hr << std::endl;
                return;
            }
        } else {
            // 使用普通 SHARED 句柄 (旧方式)
            ComPtr<IDXGIResource> dxgiRes;
            hr = sharedTex.As(&dxgiRes);
            if(FAILED(hr)) {
                std::cout << "Query IDXGIResource failed: " << std::hex << hr << std::endl;
                return;
            }

            hr = dxgiRes->GetSharedHandle(&sharedHandle);
            if(FAILED(hr)) {
                std::cout << "GetSharedHandle failed: " << std::hex << hr << std::endl;
                return;
            }
        }

        lastWidth = srcDesc.Width;
        lastHeight = srcDesc.Height;

        std::cout << "Created shared texture: " << std::dec << srcDesc.Width << "x" << srcDesc.Height
                  << " format: " << srcDesc.Format
                  << " (NT Handle: " << (isNTHandle ? "Yes" : "No") << ")" << std::endl;
    }

    // -------------------------------
    // 3️⃣ 将解码的源纹理复制到共享纹理
    // -------------------------------
    ctx->CopySubresourceRegion(
        sharedTex.Get(), 0, 0, 0, 0,
        tex, index, nullptr
    );
    ctx->Flush();

    // -------------------------------
    // 4️⃣ 获取 D3D12 设备并打开共享句柄
    // -------------------------------
    ComPtr<ID3D12Device> d3d12Device = dawn::native::d3d12::GetD3D12Device(device_.Get());
    if(!d3d12Device) {
        std::cout << "Failed to get D3D12 device from Dawn" << std::endl;
        return;
    }

    ComPtr<ID3D12Resource> d3d12Resource;
    HRESULT hr = d3d12Device->OpenSharedHandle(
        sharedHandle,
        IID_PPV_ARGS(&d3d12Resource)
    );
    if(FAILED(hr)) {
        std::cout << "OpenSharedHandle failed: " << std::hex << hr << std::endl;
        return;
    }

    // -------------------------------
    // 5️⃣ 使用 Dawn 的 SharedTextureMemory 通过 DXGI Handle 导入（GPU 零拷贝）
    // -------------------------------

    // 使用 DXGI Shared Handle 方式导入共享纹理
    // 这样避免了 CPU 拷贝，直接在 GPU 上操作

    // 创建 DXGI Shared Handle 描述符
    wgpu::SharedTextureMemoryDXGISharedHandleDescriptor handleDesc = {};
    handleDesc.handle = sharedHandle;
    handleDesc.useKeyedMutex = false;  // 不使用 keyed mutex

    wgpu::SharedTextureMemoryDescriptor stmDesc = {};
    stmDesc.nextInChain = &handleDesc;

    // 导入共享纹理内存
    wgpu::SharedTextureMemory sharedMemory = device_.ImportSharedTextureMemory(&stmDesc);
    if (!sharedMemory) {
        std::cout << "Failed to import shared texture memory via DXGI handle" << std::endl;
        return;
    }

    // 获取共享纹理属性
    wgpu::SharedTextureMemoryProperties props = {};
    sharedMemory.GetProperties(&props);

    std::cout << "Shared texture memory imported: format=" << (int)props.format
              << " usage=" << (int)props.usage
              << " size=" << props.size.width << "x" << props.size.height << std::endl;

    // 从共享内存创建Dawn纹理
    // 必须使用与 SharedTextureMemory 相同的格式
    wgpu::TextureDescriptor texDesc = {};
    texDesc.usage = props.usage;  // 使用共享纹理的 usage
    texDesc.dimension = wgpu::TextureDimension::e2D;
    texDesc.size = props.size;     // 使用共享纹理的 size
    texDesc.format = props.format; // 使用共享纹理的 format (R8BG8Biplanar420Unorm)
    texDesc.mipLevelCount = 1;
    texDesc.sampleCount = 1;

    // 创建或重用纹理
    static wgpu::Texture dawnTexture;
    static wgpu::SharedTextureMemory lastSharedMemory;
    static uint32_t texWidth = 0;
    static uint32_t texHeight = 0;

    if(!dawnTexture || texWidth != srcDesc.Width || texHeight != srcDesc.Height) {
        // Begin access
        wgpu::SharedTextureMemoryBeginAccessDescriptor beginDesc = {};
        beginDesc.initialized = true;

        dawnTexture = sharedMemory.CreateTexture(&texDesc);
        sharedMemory.BeginAccess(dawnTexture, &beginDesc);

        lastSharedMemory = sharedMemory;
        texWidth = srcDesc.Width;
        texHeight = srcDesc.Height;
    }

    // -------------------------------
    // 6️⃣ 创建 TextureView - 访问 Y 平面 (plane 0)
    // -------------------------------
    // R8BG8Biplanar420Unorm 是双平面格式：
    // - Plane 0: Y (R8Unorm)
    // - Plane 1: UV (RG8Unorm)
    wgpu::TextureViewDescriptor viewDesc = {};
    viewDesc.format = wgpu::TextureFormat::R8Unorm;  // Y 平面是 R8
    viewDesc.dimension = wgpu::TextureViewDimension::e2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;  // Plane 0 = Y
    viewDesc.arrayLayerCount = 1;
    viewDesc.aspect = wgpu::TextureAspect::Plane0Only;  // 只访问 Y 平面

    y_tex_ = dawnTexture.CreateView(&viewDesc);
}

void FrameRenderer::init_pipeline()
{
    VertexAttribute attrs[] = {
        { .format = VertexFormat::Float32x2, .offset = 0, .shaderLocation = 0 },                // pos
        { .format = VertexFormat::Float32x2, .offset = sizeof(float) * 2, .shaderLocation = 1 } // uv
    };
    VertexBufferLayout vb_layout{
        .arrayStride = sizeof(float) * 4,
        .attributeCount = 2,
        .attributes = attrs
    };

    if (!module_) {
        init_shader();
    }
    
    FragmentState fragment_state{};
    {
        ColorTargetState target{};
        target.format = surface_texture_fmt_;
        target.writeMask = ColorWriteMask::All;

        fragment_state.module = module_;
        fragment_state.entryPoint = "fs";
        fragment_state.targetCount = 1;
        fragment_state.targets = &target;
    }
   
    bind_group_layout_ = dawn::utils::MakeBindGroupLayout(
        device_,
        {
            { 0, ShaderStage::Fragment, SamplerBindingType::Filtering },   // sampler
            { 1, ShaderStage::Fragment, TextureSampleType::Float, TextureViewDimension::e2D }  // Y texture
        }
    );

    RenderPipelineDescriptor desc {
        .layout = dawn::utils::MakeBasicPipelineLayout(device_, &bind_group_layout_),
        .vertex = {
            .module = module_,
            .entryPoint = "vs",
            .bufferCount = 1,
            .buffers = &vb_layout
        },
        .fragment = &fragment_state
    };
    pipeline_ = device_.CreateRenderPipeline(&desc);
}

void FrameRenderer::render(RenderPassEncoder& pass)
{
    // 延迟初始化渲染资源
    if(!pipeline_) {
        init_pipeline();
    }
    if (!vertex_buffer_) {
        init_buffer();
    }
    if (!bind_group_) {
        init_bindgroup();
    }

    // 直接渲染 Y 分量作为灰度图像
    // Y 纹理可以来自：
    // 1. init_texture() 从 YUV 文件加载 (测试用)
    // 2. update_texture() 从硬解码 DX11 纹理转换得到
    pass.SetPipeline(pipeline_);
    pass.SetVertexBuffer(0, vertex_buffer_);
    pass.SetBindGroup(0, bind_group_);
    pass.Draw(6);
}
