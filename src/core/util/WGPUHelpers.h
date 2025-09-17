#pragma once

#include <webgpu/webgpu_cpp.h>

#include <array>
#include <initializer_list>
#include <string>
#include <vector>

#include "Constants.h"

namespace dawn::utils {

wgpu::ShaderModule CreateShaderModule(const wgpu::Device& device, const char* source);

wgpu::Buffer CreateBufferFromData(const wgpu::Device& device,
                                  const void* data,
                                  uint64_t size,
                                  wgpu::BufferUsage usage);
wgpu::TextureView CreateTextureFromPath(
                                  const wgpu::Device& device,
                                  const char* filePath
);
wgpu::Sampler CreateSamper(const wgpu::Device& device);

struct ComboRenderPassDescriptor : public wgpu::RenderPassDescriptor {
public:
    ComboRenderPassDescriptor(const std::vector<wgpu::TextureView>& colorAttachmentInfo = {},
                              wgpu::TextureView depthStencil = wgpu::TextureView());
    ~ComboRenderPassDescriptor();

    ComboRenderPassDescriptor(const ComboRenderPassDescriptor& otherRenderPass);
    const ComboRenderPassDescriptor& operator=(const ComboRenderPassDescriptor& otherRenderPass);

    void UnsetDepthStencilLoadStoreOpsForFormat(wgpu::TextureFormat format);

    std::array<wgpu::RenderPassColorAttachment, kMaxColorAttachments> cColorAttachments;
    wgpu::RenderPassDepthStencilAttachment cDepthStencilAttachmentInfo = {};
};


struct BindingLayoutEntryInitializationHelper : wgpu::BindGroupLayoutEntry {
    BindingLayoutEntryInitializationHelper(uint32_t entryBinding,
                                           wgpu::ShaderStage entryVisibility,
                                           wgpu::BufferBindingType bufferType,
                                           bool bufferHasDynamicOffset = false,
                                           uint64_t bufferMinBindingSize = 0);
    BindingLayoutEntryInitializationHelper(uint32_t entryBinding,
                                           wgpu::ShaderStage entryVisibility,
                                           wgpu::SamplerBindingType samplerType);
    BindingLayoutEntryInitializationHelper(
        uint32_t entryBinding,
        wgpu::ShaderStage entryVisibility,
        wgpu::TextureSampleType textureSampleType,
        wgpu::TextureViewDimension viewDimension = wgpu::TextureViewDimension::e2D,
        bool textureMultisampled = false);
    BindingLayoutEntryInitializationHelper(
        uint32_t entryBinding,
        wgpu::ShaderStage entryVisibility,
        wgpu::StorageTextureAccess storageTextureAccess,
        wgpu::TextureFormat format,
        wgpu::TextureViewDimension viewDimension = wgpu::TextureViewDimension::e2D);
#ifndef __EMSCRIPTEN__
    BindingLayoutEntryInitializationHelper(uint32_t entryBinding,
                                           wgpu::ShaderStage entryVisibility,
                                           wgpu::ExternalTextureBindingLayout* bindingLayout);
    BindingLayoutEntryInitializationHelper(uint32_t entryBinding,
                                           wgpu::ShaderStage entryVisibility,
                                           wgpu::TexelBufferBindingLayout* bindingLayout);
#endif  // __EMSCRIPTEN__
    // NOLINTNEXTLINE(runtime/explicit)
    BindingLayoutEntryInitializationHelper(const wgpu::BindGroupLayoutEntry& entry);
};

wgpu::BindGroupLayout MakeBindGroupLayout(
    const wgpu::Device& device,
    std::initializer_list<BindingLayoutEntryInitializationHelper> entriesInitializer);

wgpu::PipelineLayout MakeBasicPipelineLayout(
    const wgpu::Device& device,
    const wgpu::BindGroupLayout* bindGroupLayout,
    uint32_t immediateDataByteSize = 0);
}