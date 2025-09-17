#include "TextureRenderer.h"
#include <iterator>

using namespace wgpu;

void TextureRenderer::init()
{
    // -----------------------
    // 1. 顶点缓冲
    // -----------------------
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

    // -----------------------
    // 2. 采样器
    // -----------------------
    sampler_ = dawn::utils::CreateSamper(device_);

    // -----------------------
    // 3. 渲染 Shader
    // -----------------------
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

    // -----------------------
    // 4. 渲染 Pipeline
    // -----------------------
    VertexAttribute attrs[] = {
        { .format = VertexFormat::Float32x2, .offset = 0, .shaderLocation = 0 },
        { .format = VertexFormat::Float32x2, .offset = sizeof(float) * 2, .shaderLocation = 1 }
    };

    ColorTargetState target{};
    target.format = surface_texture_fmt_;
    target.writeMask = ColorWriteMask::All;

    VertexBufferLayout vb_layout{
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

    RenderPipelineDescriptor pipeline_desc{
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

    // -----------------------
    // 5. Compute Shader
    // -----------------------
    const char* computeCode = R"(
        @group(0) @binding(0) var srcTex: texture_2d<f32>;
        @group(0) @binding(1) var dstTex: texture_storage_2d<rgba8unorm, write>;

        @compute @workgroup_size(16, 16)
        fn main(@builtin(global_invocation_id) gid: vec3u) {
            let dims: vec2u = textureDimensions(srcTex).xy;
            if (gid.x >= dims.x || gid.y >= dims.y) { return; }
            let color: vec4f = textureLoad(srcTex, vec2i(gid.xy), 0);
            textureStore(dstTex, vec2i(gid.xy), vec4f(color.r, 0.0, 0.0, color.a));
        }
    )";

    ShaderSourceWGSL wgslCompute{};
    wgslCompute.code = computeCode;
    ShaderModuleDescriptor cs_desc{};
    cs_desc.nextInChain = &wgslCompute;
    ShaderModule csModule = device_.CreateShaderModule(&cs_desc);

    auto computeBGL = dawn::utils::MakeBindGroupLayout(device_, {
        {0, ShaderStage::Compute, TextureSampleType::Float, TextureViewDimension::e2D},
        {1, ShaderStage::Compute, StorageTextureAccess::WriteOnly, TextureFormat::RGBA8Unorm}
    });

    ComputePipelineDescriptor desc{
        .layout = dawn::utils::MakeBasicPipelineLayout(device_, &computeBGL),
        .compute = {.module = csModule, .entryPoint = "main" }
    };
    computePipeline_ = device_.CreateComputePipeline(&desc);
}

void TextureRenderer::setTexture(const char* path)
{
    // 原始纹理
    originalTexture_ = dawn::utils::CreateTextureFromPath(device_, path);

    // 创建处理后的存储纹理
    TextureDescriptor desc{};
    desc.size.width = 512;
    desc.size.height = 512;
    desc.size.depthOrArrayLayers = 1;
    desc.mipLevelCount = 1;
    desc.dimension = TextureDimension::e2D;
    desc.format = TextureFormat::RGBA8Unorm;
    desc.usage = TextureUsage::TextureBinding | TextureUsage::StorageBinding;
    processedTexture_ = device_.CreateTexture(&desc);
    processedTextureView_ = processedTexture_.CreateView();

    // Compute BindGroup
    BindGroupEntry entries[2]{};
    entries[0].binding = 0;
    entries[0].textureView = originalTexture_;
    entries[1].binding = 1;
    entries[1].textureView = processedTextureView_;

    BindGroupDescriptor bgDesc{
        .layout = computePipeline_.GetBindGroupLayout(0),
        .entryCount = 2,
        .entries = entries
    };
    computeBindGroup_ = device_.CreateBindGroup(&bgDesc);

    // Dispatch compute shader
    CommandEncoder encoder = device_.CreateCommandEncoder();
    ComputePassEncoder pass = encoder.BeginComputePass();
    pass.SetPipeline(computePipeline_);
    pass.SetBindGroup(0, computeBindGroup_);
    uint32_t dispatchX = (512 + 15) / 16;
    uint32_t dispatchY = (512 + 15) / 16;
    pass.DispatchWorkgroups(dispatchX, dispatchY);
    pass.End();

    wgpu::CommandBuffer cmdBuffer = encoder.Finish();
    device_.GetQueue().Submit(1, &cmdBuffer);

    // 渲染管线使用处理后的纹理
    BindGroupEntry renderEntries[2]{};
    renderEntries[0].binding = 0;
    renderEntries[0].sampler = sampler_;
    renderEntries[1].binding = 1;
    renderEntries[1].textureView = processedTextureView_;

    BindGroupDescriptor renderBGDesc{
        .layout = pipeline_.GetBindGroupLayout(0),
        .entryCount = 2,
        .entries = renderEntries
    };
    bind_group_ = device_.CreateBindGroup(&renderBGDesc);
}

void TextureRenderer::render(RenderPassEncoder& pass)
{
    pass.SetPipeline(pipeline_);
    pass.SetBindGroup(0, bind_group_);
    pass.SetVertexBuffer(0, vertex_buffer_);
    pass.Draw(6);
    pass.End();
}
