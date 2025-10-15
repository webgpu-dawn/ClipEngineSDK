#include "EquirectangularRenderer.h"
// #include "../util/stb_image.h"
#include <iostream>
#include <cmath>

using namespace wgpu;

struct ColorParams {
    float exposure = 1.0f;
};

void EquirectangularRenderer::init() {
    init_buffer();
    // init texture
    rgba_tex_ = res_factory_->createTextureFromPath("assets/kloster_weltenburg.jpg").CreateView();
    // init sampler
    sampler_ = res_factory_->createSampler();
    // init shader
    shader_ = res_factory_->createShaderFromPath("D:/TestDawn/src/core/shader/equirectangular.wgsl");
    
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

    vertex_buffer_ = res_factory_->createBuffer(
        vertex_data.data(), vertex_data.size()*sizeof(float), wgpu::BufferUsage::Vertex
    );

    index_buffer_ = res_factory_->createBuffer(
        index_data.data(), index_data.size()*sizeof(uint32_t), BufferUsage::Index
    );

    index_count_ = (uint32_t)index_data.size();
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
    fragment_state.module = shader_;
    fragment_state.entryPoint = "fs";
    fragment_state.targetCount = 1;
    fragment_state.targets = &target;

    bind_group_layout_ = res_factory_->createBindGroupLayout({
        { .binding = 0, .visibility = ShaderStage::Fragment, .sampler = { .type = SamplerBindingType::Filtering } },
        { .binding = 1, .visibility = ShaderStage::Fragment, .texture = { .sampleType = TextureSampleType::Float, .viewDimension = TextureViewDimension::e2D } },
        { .binding = 2, .visibility = ShaderStage::Fragment, .buffer = { .type = BufferBindingType::Uniform, .hasDynamicOffset = false } }
    });

    RenderPipelineDescriptor desc{};
    desc.layout = dawn::utils::MakeBasicPipelineLayout(device_, &bind_group_layout_);
    desc.vertex = { .module = shader_, .entryPoint="vs", .bufferCount=1, .buffers=&vb_layout };

    desc.fragment = &fragment_state;

    pipeline_ = device_.CreateRenderPipeline(&desc);
}

// ---------------- BindGroup ----------------
void EquirectangularRenderer::init_bindgroup() {
    uniform_buffer_ = res_factory_->createBuffer(nullptr, sizeof(ColorParams), BufferUsage::Uniform | BufferUsage::CopyDst);

    bind_group_ = res_factory_->createBindGroup(
        bind_group_layout_,
        {
            { .binding = 0, .sampler = sampler_ },
            { .binding = 1, .textureView = rgba_tex_ },
            { .binding = 2, .buffer = uniform_buffer_, .offset = 0, .size = sizeof(ColorParams) } 
        }
    );
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
