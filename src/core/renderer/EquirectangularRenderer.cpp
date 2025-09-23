#include "EquirectangularRenderer.h"
#include <iterator>
#include <fstream>
#include <vector>
#include <iostream>

#include "../util/stb_image.h"

using namespace wgpu;


void EquirectangularRenderer::init()
{
    init_texture();
}

void EquirectangularRenderer::init_buffer()
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

void EquirectangularRenderer::init_sampler()
{
    sampler_ = dawn::utils::CreateSamper(device_);
}

void EquirectangularRenderer::init_texture()
{
    // init texture
    int temp_w, temp_h, channels;
    stbi_uc* pixels = stbi_load("assets//kloster_weltenburg.jpg", &temp_w, &temp_h, &channels, 4);
    if(!pixels) {
        throw std::runtime_error("Failed to load texture image");
    }

    uint32_t w = temp_w;
    uint32_t h = temp_h;

    TextureDescriptor desc {
        .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst,
        .dimension = TextureDimension::e2D,
        .size = {
            .width = w,
            .height = h,
            .depthOrArrayLayers = 1
        },
        .format = TextureFormat::RGBA8Unorm,
        .mipLevelCount = 1,
    };
    Texture texture = device_.CreateTexture(&desc);
    rgba_tex_ = texture.CreateView();

    TexelCopyTextureInfo info;
    info.texture = texture;
    info.mipLevel = 0;
    info.origin = {0, 0, 0};

    TexelCopyBufferLayout layout;
    layout.offset = 0;
    layout.bytesPerRow = w * 4;
    layout.rowsPerImage = h;

    Extent3D copySize{};
    copySize.width = w;
    copySize.height = h;
    copySize.depthOrArrayLayers = 1;

    device_.GetQueue().WriteTexture(&info, (void*)pixels, w * h * 4, &layout, &copySize);
}

void EquirectangularRenderer::init_shader()
{
    module_ = dawn::utils::CreateShaderModuleFromePath(device_, "D:/TestDawn/src/core/shader/rgba.wgsl");
}
struct ColorParams {
};
void EquirectangularRenderer::init_bindgroup()
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

void EquirectangularRenderer::init_pipeline()
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

void EquirectangularRenderer::render(RenderPassEncoder& pass)
{
    if(!pipeline_) {
        init_pipeline();
    }
    if (!vertex_buffer_) {
        init_buffer();
    }
    if (!bind_group_) {
        init_bindgroup();
    }

    static float exposure = 0.0;
    static float step = 0.01;
    // std::cout << "Exposure = " << exposure << " " << (exposure > 1.5) << std::endl;
    if(exposure > 1.5) {
        step = -0.01;
    }
    if(exposure <= 0) {
        step = 0.01;
    }
    exposure += step;
    
    ColorParams params = {
        
    };

    // device_.GetQueue().WriteBuffer(uniform_buffer_, 0, &params, sizeof(ColorParams));

    pass.SetPipeline(pipeline_);
    pass.SetVertexBuffer(0, vertex_buffer_);
    pass.SetBindGroup(0, bind_group_);
    pass.Draw(6);
}
