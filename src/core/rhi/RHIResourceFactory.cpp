#include "RHIResourceFactory.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../util/stb_image.h"

using namespace wgpu;


Sampler RHIResourceFactory::createSampler()
{
    SamplerDescriptor desc {
        .label = "Default Sampler",
        .addressModeU = AddressMode::ClampToEdge,
        .addressModeV = AddressMode::ClampToEdge,
        .addressModeW = AddressMode::ClampToEdge,
        .magFilter = FilterMode::Linear,
        .minFilter = FilterMode::Linear,
        .mipmapFilter = MipmapFilterMode::Nearest,
        .lodMinClamp = 0.0f,
        .lodMaxClamp = 32.0f,
        .compare = CompareFunction::Undefined,
        .maxAnisotropy = 1
    };
    return device_.CreateSampler(&desc);
}

Buffer RHIResourceFactory::createBuffer(const void* data,
                                              uint64_t size,
                                              BufferUsage usage)
{
    BufferDescriptor desc {
        .label = "Default Buffer",
        .usage = usage | BufferUsage::CopyDst,
        .size = size,
        .mappedAtCreation = false
    };
    Buffer buffer = device_.CreateBuffer(&desc);
    if (data) {
        device_.GetQueue().WriteBuffer(buffer, 0, data, size);
    }
    return buffer;
}

ShaderModule RHIResourceFactory::createShaderFromPath(const char* path)
{
    std::ifstream file(path);
    if(!file.is_open()) throw std::runtime_error("Cannot open file");

    std::stringstream ss;
    ss << file.rdbuf();

    std::filesystem::path p(path);
    return createShaderFromCode(ss.str().data());
}

ShaderModule RHIResourceFactory::createShaderFromCode(const char* code)
{
    ShaderSourceWGSL wgsl;
    wgsl.code = code;

    ShaderModuleDescriptor desc;
    desc.nextInChain = &wgsl;
    desc.label = "Default Shader";
    return device_.CreateShaderModule(&desc);
}

Texture RHIResourceFactory::createTextureFromPath(const char* path)
{
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

    Texture texture = device_.CreateTexture(&desc);
    TexelCopyTextureInfo info {
        .texture = texture,
        .mipLevel = 0,
        .origin = {0, 0, 0},
    };
    TexelCopyBufferLayout layout {
        .offset = 0,
        .bytesPerRow = (uint32_t)(4 * w),
        .rowsPerImage = (uint32_t)h
    };
    Extent3D size {
        .width = (uint32_t)w,
        .height = (uint32_t)h,
        .depthOrArrayLayers = 1
    };
    device_.GetQueue().WriteTexture(&info, pixels, (size_t)(4 * w * h), &layout, &size);
    stbi_image_free(pixels);

    return texture;
}

BindGroupLayout RHIResourceFactory::createBindGroupLayout(
    std::initializer_list<BindGroupLayoutEntry> entries)
{
    BindGroupLayoutDescriptor desc{
        .label = "Custom Bind Group Layout",
        .entryCount = static_cast<uint32_t>(entries.size()),
        .entries = entries.begin()
    };
    return device_.CreateBindGroupLayout(&desc);
}

BindGroup RHIResourceFactory::createBindGroup(
    BindGroupLayout& layout,
    std::initializer_list<BindGroupEntry> entries)
{
    BindGroupDescriptor desc {
        .label = "Custom Bind Group",
        .layout = layout,
        .entryCount = static_cast<uint32_t>(entries.size()),
        .entries = entries.begin()
    };
    return device_.CreateBindGroup(&desc);
}

RenderPipeline RHIResourceFactory::createRenderPipeline()
{
    RenderPipelineDescriptor desc {
        .label = "Default Render Pipeline",
        .layout = nullptr,
        .vertex = {},
        .primitive = {},
        .depthStencil = nullptr,
        .multisample = {},
        .fragment = nullptr
    };
    return device_.CreateRenderPipeline(&desc);
}

