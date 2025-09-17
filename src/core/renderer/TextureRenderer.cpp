#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "TextureRenderer.h"

using namespace wgpu;

void TextureRenderer::init()
{
    // 1. 顶点缓冲
    float vertex_data[] = {
        -1.0f, -1.0f, 0.0f, 1.0f, // 左下
        1.0f, -1.0f, 1.0f, 1.0f, // 右下
        -1.0f,  1.0f, 0.0f, 0.0f, // 左上

        -1.0f,  1.0f, 0.0f, 0.0f, // 左上
        1.0f, -1.0f, 1.0f, 1.0f, // 右下
        1.0f,  1.0f, 1.0f, 0.0f  // 右上
    };

    BufferDescriptor vb_desc{};
    vb_desc.usage = BufferUsage::Vertex | BufferUsage::CopyDst;
    vb_desc.size = sizeof(vertex_data);
    vertex_buffer_ = device_.CreateBuffer(&vb_desc);
    device_.GetQueue().WriteBuffer(vertex_buffer_, 0, vertex_data, sizeof(vertex_data));

    // 2. 采样器
    SamplerDescriptor sampler_desc{};
    sampler_desc.magFilter = FilterMode::Linear;
    sampler_desc.minFilter = FilterMode::Linear;
    sampler_desc.addressModeU = AddressMode::ClampToEdge;
    sampler_desc.addressModeV = AddressMode::ClampToEdge;
    sampler_ = device_.CreateSampler(&sampler_desc);

    // 3. BindGroupLayout
    BindGroupLayoutEntry sampler_entry{};
    sampler_entry.binding = 0;
    sampler_entry.visibility = ShaderStage::Fragment;
    sampler_entry.sampler.type = SamplerBindingType::Filtering;

    BindGroupLayoutEntry texture_entry{};
    texture_entry.binding = 1;
    texture_entry.visibility = ShaderStage::Fragment;
    texture_entry.texture.sampleType = TextureSampleType::Float;
    texture_entry.texture.viewDimension = TextureViewDimension::e2D;
    texture_entry.texture.multisampled = false;

    BindGroupLayoutDescriptor bgl_desc{};
    BindGroupLayoutEntry entries[] = { sampler_entry, texture_entry };
    bgl_desc.entryCount = 2;
    bgl_desc.entries = entries;

    bind_group_layout_ = device_.CreateBindGroupLayout(&bgl_desc);

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
    VertexAttribute attrs[2]{};
    attrs[0].format = VertexFormat::Float32x2;
    attrs[0].offset = 0;
    attrs[0].shaderLocation = 0;
    attrs[1].format = VertexFormat::Float32x2;
    attrs[1].offset = sizeof(float) * 2;
    attrs[1].shaderLocation = 1;

    VertexBufferLayout vbLayout{};
    vbLayout.arrayStride = sizeof(float) * 4;
    vbLayout.attributeCount = 2;
    vbLayout.attributes = attrs;

    VertexState vertexState{};
    vertexState.module = module;
    vertexState.entryPoint = "vs";
    vertexState.bufferCount = 1;
    vertexState.buffers = &vbLayout;

    ColorTargetState target{};
    target.format = surface_texture_fmt_;
    target.writeMask = ColorWriteMask::All;

    FragmentState fragmentState{};
    fragmentState.module = module;
    fragmentState.entryPoint = "fs";
    fragmentState.targetCount = 1;
    fragmentState.targets = &target;

    PipelineLayoutDescriptor pipelineLayoutDesc{};
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    const BindGroupLayout bgls[] = { bind_group_layout_ };
    pipelineLayoutDesc.bindGroupLayouts = bgls;

    RenderPipelineDescriptor pipelineDesc{
        .layout = device_.CreatePipelineLayout(&pipelineLayoutDesc),
        .vertex = vertexState,
        .fragment = &fragmentState
    };

    pipeline_ = device_.CreateRenderPipeline(&pipelineDesc);
}

void TextureRenderer::setTexture(const char* path)
{
    // 1. 加载图片
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, 4);
    if (!pixels) {
        // std::cerr << "Failed to load texture: " << path << std::endl;
        return;
    }

    // 2. 创建 GPU Texture
    TextureDescriptor texDesc{};
    texDesc.usage = TextureUsage::TextureBinding | TextureUsage::CopyDst;
    texDesc.dimension = TextureDimension::e2D;
    texDesc.size.width = texWidth;
    texDesc.size.height = texHeight;
    texDesc.size.depthOrArrayLayers = 1;
    texDesc.format = TextureFormat::RGBA8Unorm;
    texDesc.mipLevelCount = 1;
    texDesc.sampleCount = 1;

    Texture texture = device_.CreateTexture(&texDesc);
    TextureView view = texture.CreateView();

    // 3. 上传像素数据
    TexelCopyTextureInfo copyTex{};
    copyTex.texture = texture;
    copyTex.mipLevel = 0;
    copyTex.origin = { 0, 0, 0 };

    TexelCopyBufferLayout dataLayout{};
    dataLayout.offset = 0;
    dataLayout.bytesPerRow = texWidth * 4; // RGBA8
    dataLayout.rowsPerImage = texHeight;

    Extent3D copySize{};
    copySize.width = texWidth;
    copySize.height = texHeight;
    copySize.depthOrArrayLayers = 1;

    device_.GetQueue().WriteTexture(&copyTex, pixels, texWidth * texHeight * 4, &dataLayout, &copySize);

    stbi_image_free(pixels);

    // 4. 创建 BindGroup
    BindGroupEntry entries[2]{};
    entries[0].binding = 0;
    entries[0].sampler = sampler_;
    entries[1].binding = 1;
    entries[1].textureView = view;

    BindGroupDescriptor bgDesc{};
    bgDesc.layout = bind_group_layout_;
    bgDesc.entryCount = 2;
    bgDesc.entries = entries;

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