#include "EquirectangularRenderer.h"
#include "../util/stb_image.h"
#include <iostream>
#include <cmath>

using namespace wgpu;

struct ColorParams {
    float exposure = 1.0f;
};

void EquirectangularRenderer::init() {
    init_texture();
    init_buffer();
    init_sampler();
    init_shader();
    init_pipeline();
    init_bindgroup();
}

const float M_PI = 3.1415926;

// ---------------- 顶点/索引缓冲 ----------------
void EquirectangularRenderer::init_buffer() {
    std::vector<float> vertex_data;
    std::vector<uint32_t> index_data;

    int latSteps = 64;
    int lonSteps = 128;

    // 顶点
    for(int lat=0; lat<=latSteps; lat++){
        float theta = (float)lat / latSteps * M_PI; // [0, π]
        for(int lon=0; lon<=lonSteps; lon++){
            float phi = (float)lon / lonSteps * 2.0f * M_PI; // [0, 2π]
            float x = sin(theta) * cos(phi);
            float y = cos(theta);
            float z = sin(theta) * sin(phi);
            vertex_data.push_back(x);
            vertex_data.push_back(y);
            vertex_data.push_back(z);
            vertex_data.push_back((float)lon/lonSteps); // uv.x
            vertex_data.push_back((float)lat/latSteps); // uv.y
        }
    }

    // 索引
    for(int lat=0; lat<latSteps; lat++){
        for(int lon=0; lon<lonSteps; lon++){
            uint32_t current = lat*(lonSteps+1) + lon;
            uint32_t next = (lat+1)*(lonSteps+1) + lon;

            index_data.push_back(current);
            index_data.push_back(next);
            index_data.push_back(current+1);

            index_data.push_back(current+1);
            index_data.push_back(next);
            index_data.push_back(next+1);
        }
    }

    vertex_buffer_ = dawn::utils::CreateBufferFromData(
        device_, vertex_data.data(), vertex_data.size()*sizeof(float), BufferUsage::Vertex
    );

    index_buffer_ = dawn::utils::CreateBufferFromData(
        device_, index_data.data(), index_data.size()*sizeof(uint32_t), BufferUsage::Index
    );

    index_count_ = (uint32_t)index_data.size();
}

// ---------------- 纹理 ----------------
void EquirectangularRenderer::init_texture() {
    int w, h, channels;
    stbi_uc* pixels = stbi_load("assets/kloster_weltenburg.jpg", &w, &h, &channels, 4);
    if(!pixels) throw std::runtime_error("Failed to load texture");

    TextureDescriptor desc{
        .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst,
        .dimension = TextureDimension::e2D,
        .size = { .width=(uint32_t)w, .height=(uint32_t)h, .depthOrArrayLayers=1 },
        .format = TextureFormat::RGBA8Unorm,
        .mipLevelCount = 1
    };
    Texture tex = device_.CreateTexture(&desc);
    rgba_tex_ = tex.CreateView();

    TexelCopyTextureInfo info{ .texture=tex, .mipLevel=0, .origin={0,0,0} };
    TexelCopyBufferLayout layout{ .offset=0, .bytesPerRow=(uint32_t)w*4, .rowsPerImage=(uint32_t)h };
    Extent3D copySize{ .width=(uint32_t)w, .height=(uint32_t)h, .depthOrArrayLayers=1 };

    device_.GetQueue().WriteTexture(&info, pixels, w*h*4, &layout, &copySize);
    stbi_image_free(pixels);
}

// ---------------- 采样器 ----------------
void EquirectangularRenderer::init_sampler() {
    sampler_ = dawn::utils::CreateSamper(device_);
}

// ---------------- Shader ----------------
void EquirectangularRenderer::init_shader() {
    module_ = dawn::utils::CreateShaderModuleFromePath(device_, "D:/TestDawn/src/core/shader/equirectangular.wgsl");
}

// ---------------- Pipeline ----------------
void EquirectangularRenderer::init_pipeline() {
    VertexAttribute attrs[] = {
        { .format=VertexFormat::Float32x3, .offset=0, .shaderLocation=0 }, // position
        { .format=VertexFormat::Float32x2, .offset=sizeof(float)*3, .shaderLocation=1 } // uv
    };
    VertexBufferLayout vb_layout{ .arrayStride=sizeof(float)*5, .attributeCount=2, .attributes=attrs };

    FragmentState fragment_state{};
    ColorTargetState target{};
    target.format = surface_texture_fmt_;
    target.writeMask = ColorWriteMask::All;
    fragment_state.module = module_;
    fragment_state.entryPoint = "fs";
    fragment_state.targetCount = 1;
    fragment_state.targets = &target;

    bind_group_layout_ = dawn::utils::MakeBindGroupLayout(device_, {
        {0, ShaderStage::Fragment, SamplerBindingType::Filtering},          // sampler
        {1, ShaderStage::Fragment, TextureSampleType::Float, TextureViewDimension::e2D}, // texture
        {2, ShaderStage::Fragment, BufferBindingType::Uniform, false}       // uniform
    });

    RenderPipelineDescriptor desc{};
    desc.layout = dawn::utils::MakeBasicPipelineLayout(device_, &bind_group_layout_);
    desc.vertex = { .module=module_, .entryPoint="vs", .bufferCount=1, .buffers=&vb_layout };
    desc.fragment = &fragment_state;

    pipeline_ = device_.CreateRenderPipeline(&desc);
}

// ---------------- BindGroup ----------------
void EquirectangularRenderer::init_bindgroup() {
    BufferDescriptor desc{};
    desc.size = sizeof(ColorParams);
    desc.usage = BufferUsage::Uniform | BufferUsage::CopyDst;
    uniform_buffer_ = device_.CreateBuffer(&desc);

    BindGroupEntry entries[3]{};
    entries[0].binding = 0; entries[0].sampler = sampler_;
    entries[1].binding = 1; entries[1].textureView = rgba_tex_;
    entries[2].binding = 2; entries[2].buffer = uniform_buffer_;
    entries[2].offset = 0; entries[2].size = sizeof(ColorParams);

    BindGroupDescriptor bgDesc{};
    bgDesc.layout = bind_group_layout_;
    bgDesc.entryCount = 3;
    bgDesc.entries = entries;

    bind_group_ = device_.CreateBindGroup(&bgDesc);
}

// ---------------- 渲染 ----------------
void EquirectangularRenderer::render(RenderPassEncoder& pass) {
    ColorParams params{};
    device_.GetQueue().WriteBuffer(uniform_buffer_, 0, &params, sizeof(ColorParams));

    pass.SetPipeline(pipeline_);
    pass.SetVertexBuffer(0, vertex_buffer_);
    pass.SetIndexBuffer(index_buffer_, IndexFormat::Uint32, 0, index_count_ * sizeof(uint32_t));
    pass.SetBindGroup(0, bind_group_);
    pass.DrawIndexed(index_count_);
}
