#include "TextureRenderer.h"

#include <iterator>

using namespace wgpu;

void TextureRenderer::init()
{
    // 1. 顶点缓冲
    float vertex_data[] = {
        -1.0f, -1.0f, 0.0f, 1.0f, // 左下
        1.0f, -1.0f, 1.0f, 1.0f,  // 右下
        -1.0f,  1.0f, 0.0f, 0.0f, // 左上

        -1.0f,  1.0f, 0.0f, 0.0f, // 左上
        1.0f, -1.0f, 1.0f, 1.0f,  // 右下
        1.0f,  1.0f, 1.0f, 0.0f   // 右上
    };

    vertex_buffer_ = dawn::utils::CreateBufferFromData(device_, vertex_data, sizeof(vertex_data), BufferUsage::Vertex);

    // 2. 采样器
    sampler_ = dawn::utils::CreateSamper(device_);

    // 4. Shader
    const char* code = R"(
        @group(0) @binding(0) var mySampler: sampler;
        @group(0) @binding(1) var myTexture: texture_2d<f32>;

        struct VertexOutput {
            @builtin(position) pos: vec4f,
            @location(0) fragUV: vec2f
        };

        @vertex
        fn vs(@location(0) pos: vec2f, @location(1) uv: vec2f) -> VertexOutput {
            var out: VertexOutput;
            out.pos = vec4f(pos.x, pos.y, 0.0, 1.0);
            out.fragUV = uv;
            return out;
        }

        @fragment
        fn fs(input: VertexOutput) -> @location(0) vec4f {
            return textureSample(myTexture, mySampler, input.fragUV);
        }
    )";

    ShaderSourceWGSL wgsl{};
    wgsl.code = code;
    ShaderModuleDescriptor sm_desc{};
    sm_desc.nextInChain = &wgsl;
    ShaderModule module = device_.CreateShaderModule(&sm_desc);

    // 5. Pipeline
    VertexAttribute attrs[] = {
        { .format = VertexFormat::Float32x2, .offset = 0, .shaderLocation = 0 },
        { .format = VertexFormat::Float32x2, .offset = sizeof(float) * 2, .shaderLocation = 1 }
    };


    ColorTargetState target{};
    target.format = surface_texture_fmt_;
    target.writeMask = ColorWriteMask::All;

    VertexBufferLayout vb_layout {
        .arrayStride = sizeof(float) * 4,
        .attributeCount = 2,
        .attributes = attrs
    };
    FragmentState fragment_state{};
    fragment_state.module = module;
    fragment_state.entryPoint = "fs";
    fragment_state.targetCount = 1;
    fragment_state.targets = &target;

    bind_group_layout_ = dawn::utils::MakeBindGroupLayout(
        device_,
        {
            { 0, ShaderStage::Fragment, SamplerBindingType::Filtering },                      
            { 1, ShaderStage::Fragment, TextureSampleType::Float, TextureViewDimension::e2D }
        }
    );

    RenderPipelineDescriptor pipeline_desc {
        .layout = dawn::utils::MakeBasicPipelineLayout(device_, &bind_group_layout_),
        .vertex = {
            .module = module,
            .entryPoint = "vs",
            .bufferCount = 1,
            .buffers = &vb_layout
        },
        .fragment = &fragment_state
    };

    pipeline_ = device_.CreateRenderPipeline(&pipeline_desc);

}

void TextureRenderer::setTexture(const char* path)
{
    BindGroupEntry entries[2]{};
    entries[0].binding = 0;
    entries[0].sampler = sampler_;
    entries[1].binding = 1;
    entries[1].textureView = dawn::utils::CreateTextureFromPath(device_, path);

    BindGroupDescriptor bgDesc{
        .layout = pipeline_.GetBindGroupLayout(0),
        .entryCount = 2,
        .entries = entries
    };

    bind_group_ = device_.CreateBindGroup(&bgDesc);
}

void TextureRenderer::render(wgpu::RenderPassEncoder& pass)
{
    pass.SetPipeline(pipeline_);
    pass.SetBindGroup(0, bind_group_);
    pass.SetVertexBuffer(0, vertex_buffer_);
    pass.Draw(6);
    pass.End();
}