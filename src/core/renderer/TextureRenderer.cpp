#include "TextureRenderer.h"
#include <iterator>
#include <fstream>
#include <vector>

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
        @group(0) @binding(1) var texture_y: texture_2d<f32>;
        @group(0) @binding(2) var texture_u: texture_2d<f32>;
        @group(0) @binding(3) var texture_v: texture_2d<f32>;

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
            // 取 Y/U/V 分量
            let y = textureSample(texture_y, mySampler, input.fragUV).r;
            let u = textureSample(texture_u, mySampler, input.fragUV).r;
            let v = textureSample(texture_v, mySampler, input.fragUV).r;

            // 偏移到 [-0.5, +0.5]
            let u_shifted = u - 0.5;
            let v_shifted = v - 0.5;

            // BT.601 YUV 转换到 RGB
            let r = y + 1.402 * v_shifted;
            let g = y - 0.344136 * u_shifted - 0.714136 * v_shifted;
            let b = y + 1.772 * u_shifted;

            return vec4f(r, g, b, 1.0);
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
            { 0, ShaderStage::Fragment, SamplerBindingType::Filtering },   // sampelr
            { 1, ShaderStage::Fragment, TextureSampleType::Float, TextureViewDimension::e2D }, // texture
            { 2, ShaderStage::Fragment, TextureSampleType::Float, TextureViewDimension::e2D }, // texture
            { 3, ShaderStage::Fragment, TextureSampleType::Float, TextureViewDimension::e2D } // texture
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
    // 加载 yuv 数据
    uint32_t width = 5888;
    uint32_t height= 3840;

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

    // 渲染管线使用处理后的纹理
    BindGroupEntry renderEntries[4]{};
    renderEntries[0].binding = 0;
    renderEntries[0].sampler = sampler_;
    renderEntries[1].binding = 1;
    renderEntries[1].textureView = tex_y.CreateView();
    renderEntries[2].binding = 2;
    renderEntries[2].textureView = tex_u.CreateView();
    renderEntries[3].binding = 3;
    renderEntries[3].textureView = tex_v.CreateView();


    BindGroupDescriptor renderBGDesc{
        .layout = pipeline_.GetBindGroupLayout(0),
        .entryCount = 4,
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
}
