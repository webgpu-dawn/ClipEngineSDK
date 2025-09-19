#include "TextureRenderer.h"
#include <iterator>
#include <fstream>
#include <vector>
#include <iostream>

using namespace wgpu;

uint32_t width = 5888;
uint32_t height= 3840;

void TextureRenderer::init()
{
    init_texture();
}

void TextureRenderer::init_buffer()
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

void TextureRenderer::init_sampler()
{
    sampler_ = dawn::utils::CreateSamper(device_);
}

void TextureRenderer::init_texture()
{
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

void TextureRenderer::init_shader()
{
    module_ = dawn::utils::CreateShaderModuleFromePath(device_, "D:/TestDawn/src/core/shader/color_adjust.wgsl");
}
struct ColorParams {
    float exposure;
    float contrast;
};
void TextureRenderer::init_bindgroup()
{
    // 渲染管线使用处理后的纹理
    if (!sampler_) {
        init_sampler();
    }

    
    wgpu::BufferDescriptor desc;
    desc.size = sizeof(ColorParams);
    desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    uniform_buffer_ = device_.CreateBuffer(&desc);

    BindGroupEntry entries[3]{};
    entries[0].binding = 0;
    entries[0].sampler = sampler_;
    entries[1].binding = 1;
    entries[1].textureView = rgba_tex_;
    entries[2].binding = 2;
    entries[2].buffer = uniform_buffer_;
    entries[2].offset = 0;
    entries[2].size = sizeof(ColorParams);

    BindGroupDescriptor renderBGDesc{
        .layout = pipeline_.GetBindGroupLayout(0),
        .entryCount = 3,
        .entries = entries
    };
    bind_group_ = device_.CreateBindGroup(&renderBGDesc);
}

void TextureRenderer::init_compute_pipeline_and_bind_group()
{
    ShaderModule module = dawn::utils::CreateShaderModuleFromePath(device_, "D:/TestDawn/src/core/shader/yuv420p_to_rgba_compute.wgsl");

    auto bgl = dawn::utils::MakeBindGroupLayout(
        device_,
        {
            { 0, ShaderStage::Compute, TextureSampleType::Float, TextureViewDimension::e2D },
            { 1, ShaderStage::Compute, TextureSampleType::Float, TextureViewDimension::e2D },
            { 2, ShaderStage::Compute, TextureSampleType::Float, TextureViewDimension::e2D },
            { 3, ShaderStage::Compute, StorageTextureAccess::WriteOnly, TextureFormat::RGBA8Unorm }
        }
    );

    ComputePipelineDescriptor desc {
        .label = "Compute Pipeline",
        .layout = dawn::utils::MakeBasicPipelineLayout(device_, &bgl),
        .compute = {
            .module = module,
            .entryPoint = "main"
        }
    };
    compute_pipeline_ = device_.CreateComputePipeline(&desc);

    BindGroupDescriptor cbg_desc;
    
        TextureDescriptor tex_desc {
            .usage = TextureUsage::TextureBinding | TextureUsage::StorageBinding,
            .dimension = TextureDimension::e2D,
            .size = {
                .width = width,
                .height = height,
                .depthOrArrayLayers = 1
            },
            .format = TextureFormat::RGBA8Unorm,
            .mipLevelCount = 1,
        };
        rgba_tex_ = device_.CreateTexture(&tex_desc).CreateView();
        BindGroupEntry entries[4] {
            { .binding = 0, .textureView = y_tex_ },
            { .binding = 1, .textureView = u_tex_ },
            { .binding = 2, .textureView = v_tex_ },
            { .binding = 3, .textureView = rgba_tex_ }
        };
        cbg_desc.label = "compute_bind_group";
        cbg_desc.layout = compute_pipeline_.GetBindGroupLayout(0);
        cbg_desc.entryCount = 4;
        cbg_desc.entries = entries;
    
    computeBindGroup_ = device_.CreateBindGroup(&cbg_desc);
}

void TextureRenderer::init_pipeline()
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
            { 0, ShaderStage::Fragment, SamplerBindingType::Filtering },   // sampelr
            { 1, ShaderStage::Fragment, TextureSampleType::Float, TextureViewDimension::e2D },
            { 2, ShaderStage::Fragment, BufferBindingType::Uniform, false }
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

void TextureRenderer::render(RenderPassEncoder& pass)
{
    if(!pipeline_) {
        init_pipeline();
    }
    if (!vertex_buffer_) {
        init_buffer();
    }
    if(!computeBindGroup_) {
        init_compute_pipeline_and_bind_group();
    }
    if (!bind_group_) {
        init_bindgroup();
    }

    {
        CommandEncoder encoder = device_.CreateCommandEncoder();
        ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(compute_pipeline_);
        pass.SetBindGroup(0, computeBindGroup_);
        uint32_t dispatchX = (width + 15) / 16;
        uint32_t dispatchY = (height + 15) / 16;
        pass.DispatchWorkgroups(dispatchX, dispatchY);
        pass.End();

        wgpu::CommandBuffer cmdBuffer = encoder.Finish();
        device_.GetQueue().Submit(1, &cmdBuffer);
    }

    static float exposure = 0.0;
    static float step = 0.01;
    std::cout << "Exposure = " << exposure << " " << (exposure > 1.5) << std::endl;
    if(exposure > 1.5) {
        step = -0.01;
    }
    if(exposure <= 0) {
        step = 0.01;
    }
    exposure += step;
    
    ColorParams params = {
        .exposure = exposure,
        .contrast = exposure
    };

    device_.GetQueue().WriteBuffer(uniform_buffer_, 0, &params, sizeof(ColorParams));

    pass.SetPipeline(pipeline_);
    pass.SetVertexBuffer(0, vertex_buffer_);
    pass.SetBindGroup(0, bind_group_);
    pass.Draw(6);
}
