#include "WGPUHelpers.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <fstream>
#include <sstream>
#include <filesystem>

using namespace wgpu;

namespace dawn::utils {

wgpu::ShaderModule CreateShaderModule(const wgpu::Device& device, const char* source, std::string label)
{
    wgpu::ShaderSourceWGSL wgsl;
    wgsl.code = source;
    wgpu::ShaderModuleDescriptor desc;
    desc.nextInChain = &wgsl;
    desc.label = label.data();
    return device.CreateShaderModule(&desc);
}

wgpu::ShaderModule CreateShaderModuleFromePath(const wgpu::Device& device, const char* path, std::string label)
{
    std::ifstream file(path);
    if(!file.is_open()) throw std::runtime_error("Cannot open file");

    std::stringstream ss;
    ss << file.rdbuf();

    std::filesystem::path p(path);
    if(label.empty()) {
        label = p.stem().string();
    }
    
    return CreateShaderModule(device, ss.str().data(), label);
}

wgpu::Sampler CreateSamper(const wgpu::Device& device)
{
    wgpu::SamplerDescriptor desc;
    desc.magFilter = wgpu::FilterMode::Linear;
    desc.minFilter = wgpu::FilterMode::Linear;
    desc.addressModeU = AddressMode::ClampToEdge;
    desc.addressModeV = AddressMode::ClampToEdge;

    return device.CreateSampler(&desc);
}

wgpu::Buffer CreateBufferFromData(const wgpu::Device& device,
                                  const void* data,
                                  uint64_t size,
                                  wgpu::BufferUsage usage) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = size;
    descriptor.usage = usage | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    device.GetQueue().WriteBuffer(buffer, 0, data, size);
    return buffer;
}

wgpu::TextureView CreateTextureFromPath(
                                  const wgpu::Device& device,
                                  const char* filePath
) {
    // 1. 加载图片
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filePath, &texWidth, &texHeight, &texChannels, 4);
    if (!pixels) {
        // std::cerr << "Failed to load texture: " << path << std::endl;
        return wgpu::TextureView{};
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

    Texture texture = device.CreateTexture(&texDesc);
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

    device.GetQueue().WriteTexture(&copyTex, pixels, texWidth * texHeight * 4, &dataLayout, &copySize);

    stbi_image_free(pixels);

    return view;
}

ComboRenderPassDescriptor::ComboRenderPassDescriptor(
    const std::vector<wgpu::TextureView>& colorAttachmentInfo,
    wgpu::TextureView depthStencil) {
    for (uint32_t i = 0; i < kMaxColorAttachments; ++i) {
        cColorAttachments[i].loadOp = wgpu::LoadOp::Clear;
        cColorAttachments[i].storeOp = wgpu::StoreOp::Store;
        cColorAttachments[i].clearValue = {0.0f, 0.0f, 0.0f, 0.0f};
    }

    cDepthStencilAttachmentInfo.depthClearValue = 1.0f;
    cDepthStencilAttachmentInfo.stencilClearValue = 0;
    cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Clear;
    cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Store;
    cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Clear;
    cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Store;

    colorAttachmentCount = colorAttachmentInfo.size();
    uint32_t colorAttachmentIndex = 0;
    for (const wgpu::TextureView& colorAttachment : colorAttachmentInfo) {
        if (colorAttachment.Get() != nullptr) {
            cColorAttachments[colorAttachmentIndex].view = colorAttachment;
        }
        ++colorAttachmentIndex;
    }
    colorAttachments = cColorAttachments.data();

    if (depthStencil.Get() != nullptr) {
        cDepthStencilAttachmentInfo.view = depthStencil;
        depthStencilAttachment = &cDepthStencilAttachmentInfo;
    } else {
        depthStencilAttachment = nullptr;
    }
}

ComboRenderPassDescriptor::~ComboRenderPassDescriptor() = default;

ComboRenderPassDescriptor::ComboRenderPassDescriptor(const ComboRenderPassDescriptor& other) {
    *this = other;
}

const ComboRenderPassDescriptor& ComboRenderPassDescriptor::operator=(
    const ComboRenderPassDescriptor& otherRenderPass) {
    cDepthStencilAttachmentInfo = otherRenderPass.cDepthStencilAttachmentInfo;
    cColorAttachments = otherRenderPass.cColorAttachments;
    colorAttachmentCount = otherRenderPass.colorAttachmentCount;

    colorAttachments = cColorAttachments.data();

    if (otherRenderPass.depthStencilAttachment != nullptr) {
        // Assign desc.depthStencilAttachment to this->depthStencilAttachmentInfo;
        depthStencilAttachment = &cDepthStencilAttachmentInfo;
    } else {
        depthStencilAttachment = nullptr;
    }

    return *this;
}
void ComboRenderPassDescriptor::UnsetDepthStencilLoadStoreOpsForFormat(wgpu::TextureFormat format) {
    switch (format) {
        case wgpu::TextureFormat::Depth24Plus:
        case wgpu::TextureFormat::Depth32Float:
        case wgpu::TextureFormat::Depth16Unorm:
            cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Undefined;
            cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Undefined;
            break;
        case wgpu::TextureFormat::Stencil8:
            cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Undefined;
            cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Undefined;
            break;
        default:
            break;
    }
}

wgpu::BindGroupLayout MakeBindGroupLayout(
    const wgpu::Device& device,
    std::initializer_list<BindingLayoutEntryInitializationHelper> entriesInitializer) {
    std::vector<wgpu::BindGroupLayoutEntry> entries;
    for (const BindingLayoutEntryInitializationHelper& entry : entriesInitializer) {
        entries.push_back(entry);
    }

    wgpu::BindGroupLayoutDescriptor descriptor;
    descriptor.entryCount = entries.size();
    descriptor.entries = entries.data();
    return device.CreateBindGroupLayout(&descriptor);
}

wgpu::PipelineLayout MakeBasicPipelineLayout(const wgpu::Device& device,
                                             const wgpu::BindGroupLayout* bindGroupLayout,
                                             uint32_t immediateDataByteSize) {
    wgpu::PipelineLayoutDescriptor descriptor;
    if (bindGroupLayout != nullptr) {
        descriptor.bindGroupLayoutCount = 1;
        descriptor.bindGroupLayouts = bindGroupLayout;
    } else {
        descriptor.bindGroupLayoutCount = 0;
        descriptor.bindGroupLayouts = nullptr;
    }

    if (immediateDataByteSize > 0) {
        descriptor.immediateSize = immediateDataByteSize;
    }

    return device.CreatePipelineLayout(&descriptor);
}

BindingLayoutEntryInitializationHelper::BindingLayoutEntryInitializationHelper(
    uint32_t entryBinding,
    wgpu::ShaderStage entryVisibility,
    wgpu::BufferBindingType bufferType,
    bool bufferHasDynamicOffset,
    uint64_t bufferMinBindingSize) {
    binding = entryBinding;
    visibility = entryVisibility;
    buffer.type = bufferType;
    buffer.hasDynamicOffset = bufferHasDynamicOffset;
    buffer.minBindingSize = bufferMinBindingSize;
}

BindingLayoutEntryInitializationHelper::BindingLayoutEntryInitializationHelper(
    uint32_t entryBinding,
    wgpu::ShaderStage entryVisibility,
    wgpu::SamplerBindingType samplerType) {
    binding = entryBinding;
    visibility = entryVisibility;
    sampler.type = samplerType;
}

BindingLayoutEntryInitializationHelper::BindingLayoutEntryInitializationHelper(
    uint32_t entryBinding,
    wgpu::ShaderStage entryVisibility,
    wgpu::TextureSampleType textureSampleType,
    wgpu::TextureViewDimension textureViewDimension,
    bool textureMultisampled) {
    binding = entryBinding;
    visibility = entryVisibility;
    texture.sampleType = textureSampleType;
    texture.viewDimension = textureViewDimension;
    texture.multisampled = textureMultisampled;
}

BindingLayoutEntryInitializationHelper::BindingLayoutEntryInitializationHelper(
    uint32_t entryBinding,
    wgpu::ShaderStage entryVisibility,
    wgpu::StorageTextureAccess storageTextureAccess,
    wgpu::TextureFormat format,
    wgpu::TextureViewDimension textureViewDimension) {
    binding = entryBinding;
    visibility = entryVisibility;
    storageTexture.access = storageTextureAccess;
    storageTexture.format = format;
    storageTexture.viewDimension = textureViewDimension;
}

// ExternalTextureBindingLayout never contains data, so just make one that can be reused instead
// of declaring a new one every time it's needed.
wgpu::ExternalTextureBindingLayout kExternalTextureBindingLayout = {};
wgpu::TexelBufferBindingLayout kTexelBufferBindingLayout = {};

BindingLayoutEntryInitializationHelper::BindingLayoutEntryInitializationHelper(
    uint32_t entryBinding,
    wgpu::ShaderStage entryVisibility,
    wgpu::ExternalTextureBindingLayout* bindingLayout) {
    binding = entryBinding;
    visibility = entryVisibility;
    nextInChain = bindingLayout;
}

BindingLayoutEntryInitializationHelper::BindingLayoutEntryInitializationHelper(
    uint32_t entryBinding,
    wgpu::ShaderStage entryVisibility,
    wgpu::TexelBufferBindingLayout* bindingLayout) {
    binding = entryBinding;
    visibility = entryVisibility;
    nextInChain = bindingLayout;
}


}